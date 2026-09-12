// The solve loop: tolerance/top-layer search over z3, the access-point
// masking post-pass, and result extraction.
#include "solve.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>

#include "json.hpp"
#include "netpoints.hpp"
#include "smt.hpp"
#include "variables.hpp"
#include "z3++.h"

namespace aumedal {
namespace {

using Arr = std::array<long, 3>;

constexpr const char* kDummyNetToken = "_-_";
constexpr const char* kDummyNetEscaped = "_DUMMYNET_";

std::string smt2_escape_identifier(std::string s) {
    std::string::size_type p = 0;
    while ((p = s.find(kDummyNetToken, p)) != std::string::npos) {
        s.replace(p, 3, kDummyNetEscaped);
        p += 10;
    }
    return s;
}

void append(std::vector<z3::expr>& all, std::vector<z3::expr>&& v) {
    for (auto& e : v) all.push_back(std::move(e));
}

Arr parse_xyz(const std::string& s, size_t pos) {
    size_t yp = s.find('y', pos), zp = s.find('z', yp);
    return {std::stol(s.substr(pos + 1, yp - pos - 1)),
            std::stol(s.substr(yp + 1, zp - yp - 1)), std::stol(s.substr(zp + 1))};
}

void recompute_route_metrics(RoutingResult& r) {
    r.via_count = 0;
    r.total_length = 0;
    for (const auto& e : r.metals) {
        if (e.first[2] != e.second[2]) ++r.via_count;
        else r.total_length += std::labs(e.first[0] - e.second[0]) + std::labs(e.first[1] - e.second[1]);
    }
}

std::optional<ViaEnc> try_parse_via_enc(const std::string& nm) {
    if (nm.rfind("VIA_ENC_", 0) != 0) return std::nullopt;
    size_t gp = nm.find("_G_x");
    if (gp == std::string::npos) return std::nullopt;
    const int dir = (nm.find("_HOR_") != std::string::npos) ? 0 : 1;
    const int level = (nm.find("_L_G_") != std::string::npos) ? 0 : 1;
    auto p = parse_xyz(nm, gp + 3);
    return ViaEnc{p[0], p[1], p[2], dir, level};
}

std::optional<ExtPin> try_parse_ext_pin(const std::string& nm) {
    if (nm.rfind("EXT_PIN_", 0) != 0) return std::nullopt;
    std::vector<std::string> parts;
    std::string cur;
    for (char c : nm) { if (c == '_') { parts.push_back(cur); cur.clear(); } else cur += c; }
    parts.push_back(cur);
    if (parts.size() != 6) return std::nullopt;
    return ExtPin{parts[2], std::stol(parts[3]), std::stol(parts[4]), std::stol(parts[5])};
}

std::vector<z3::expr> collect_graph_constraints(SmtModel& smt, const Config& cfg,
                                                   const std::vector<std::vector<long>>& xp,
                                                   const std::vector<std::vector<long>>& yp,
                                                   const std::vector<std::vector<long>>& ext_pin_y_tracks,
                                                   const RoutingGraph& g, std::vector<Net>& cnets,
                                                   PreLayout& pre, const NetOrders& no) {
    std::vector<z3::expr> all;
    append(all, smt.tip_side_exclusivity(g.nodes));
    append(all, smt.metal_direction(xp, yp, g));
    append(all, smt.via_helper(cfg, xp, yp, g));
    append(all, smt.tip_helper(cfg, xp, yp, g));
    append(all, smt.side_helper(cfg, xp, yp, g));
    append(all, smt.corner_helper(cfg, xp, yp, g));
    append(all, smt.via_enclosure(cfg, xp, yp, g));
    append(all, smt.cross_layer_via_block(cfg, xp, yp, g));
    append(all, smt.minimum_area(cfg, xp, yp, g));
    append(all, smt.add_side_to_side_spacing(cfg, xp, yp, g));
    append(all, smt.add_tip_to_tip_spacing(cfg, xp, yp, g));
    append(all, smt.add_side_to_tip_spacing(cfg, xp, yp, g));
    append(all, smt.add_corner_spacing(cfg, xp, yp, g));
    append(all, smt.add_via_spacing(cfg, xp, yp, g));
    long num_poly = ((long)no.n_net.size() + 1) / 2;
    pre.build_blockage(cfg, no, cnets, num_poly, 1);
    append(all, smt.add_pre_layout_blockage(cfg, xp, yp, g, pre));
    return all;
}

std::vector<z3::expr> collect_net_constraints(SmtModel& smt, const Config& cfg,
                                                 const std::vector<std::vector<long>>& xp,
                                                 const std::vector<std::vector<long>>& yp,
                                                 const RoutingGraph& g, std::vector<Net>& cnets) {
    std::vector<z3::expr> all;
    append(all, smt.add_layer_exclusivity(cfg, xp, yp, g, cnets));
    append(all, smt.add_edge_assignment(cfg, g, cnets));
    append(all, smt.add_vertex_exclusivity(cfg, g, cnets));
    append(all, smt.add_metal_segment(cfg, g, cnets));
    append(all, smt.add_pin_grid_lock(cfg, g, cnets));
    append(all, smt.add_commodity_flow(cfg, g, cnets));
    append(all, smt.add_minimum_pin_length(cfg, xp, yp, g, cnets));
    return all;
}

std::vector<long> tolerance_schedule(const Config& cfg, const std::vector<std::vector<long>>& xp,
                                     const std::vector<std::vector<long>>& yp) {
    long max_tol = cfg.option.value("max_tolerance", 0L);
    long step = cfg.option.value("tolerance_step", 3L);
    if (step <= 0) step = 1;

    long minx = std::numeric_limits<long>::max(), maxx = std::numeric_limits<long>::min();
    long miny = std::numeric_limits<long>::max(), maxy = std::numeric_limits<long>::min();
    bool any = false;
    for (const auto& layer : xp) for (long x : layer) { any = true; minx = std::min(minx, x); maxx = std::max(maxx, x); }
    for (const auto& layer : yp) for (long y : layer) { miny = std::min(miny, y); maxy = std::max(maxy, y); }
    long exhaustive = 0;
    if (any) {
        const long xu = std::max(1L, (long)cfg.x_unit), yu = std::max(1L, (long)cfg.y_unit);
        const long tx = (long)((double)(maxx - minx) / (double)xu) + 2;
        const long ty = (long)((double)(maxy - miny) / (double)yu) + 2;
        exhaustive = std::max(tx, ty);
    }
    std::set<long> vals;
    if (max_tol > 0) {
        for (long t = 0; t <= max_tol; t += step) vals.insert(t);
        if (exhaustive > max_tol) vals.insert(exhaustive);
    } else {
        vals.insert(exhaustive);
    }
    if (vals.empty()) vals.insert(0);
    return std::vector<long>(vals.begin(), vals.end());
}

struct NInfo {
    const Net* net;
    std::string cname;
    bool is_power, is_dummy, is_ext_pin;
    int comm;
    double minx, maxx, miny, maxy;
    std::set<Arr> pinset;
};
NInfo make_ninfo(const Net& n, long tol, double xu, double yu,
                 const std::string& pwr, const std::string& gnd) {
    NInfo ni;
    ni.net = &n;
    ni.cname = (n.is_power ? (n.name.find(pwr) != std::string::npos ? pwr
                            : n.name.find(gnd) != std::string::npos ? gnd : n.name)
                           : n.name);
    ni.is_power = n.is_power;
    ni.is_dummy = (n.name == DUMMY_NET);
    ni.is_ext_pin = n.is_ext_pin;
    ni.comm = (int)n.pins.size() - 1;
    long bx0 = std::numeric_limits<long>::max(), by0 = std::numeric_limits<long>::max(), bx1 = 0, by1 = 0;
    for (const Pin& p : n.pins)
        for (const Point& pt : p.points) {
            bx0 = std::min(bx0, pt.x); bx1 = std::max(bx1, pt.x);
            by0 = std::min(by0, pt.y); by1 = std::max(by1, pt.y);
        }
    if (n.is_power) { ni.minx = 0; ni.maxx = 1e18; ni.miny = by0; ni.maxy = by1; }
    else {
        const long ex = ni.is_dummy ? 0 : tol;
        ni.minx = bx0 - ex * xu; ni.maxx = bx1 + ex * xu;
        ni.miny = by0 - ex * yu; ni.maxy = by1 + ex * yu;
    }
    if (ni.is_dummy)
        for (const Pin& p : n.pins)
            for (const Point& pt : p.points) ni.pinset.insert({pt.x, pt.y, pt.z});
    return ni;
}
bool in_box(const NInfo& ni, const Arr& p) {
    return (double)p[0] >= ni.minx && (double)p[0] <= ni.maxx && (double)p[1] >= ni.miny && (double)p[1] <= ni.maxy;
}

class DisjointSet {
public:
    Arr find(const Arr& x) {
        auto it = parent_.find(x);
        if (it == parent_.end()) { parent_[x] = x; return x; }
        if (it->second == x) return x;
        Arr root = find(it->second);
        parent_[x] = root;
        return root;
    }
    void unite(const Arr& a, const Arr& b) {
        Arr ra = find(a), rb = find(b);
        if (ra != rb) parent_[ra] = rb;
    }
    bool contains(const Arr& x) const { return parent_.count(x) > 0; }

private:
    std::map<Arr, Arr> parent_;
};

using EP = std::pair<Arr, Arr>;
using CommTrueFn = std::function<bool(const std::string&, int, const Arr&, const Arr&)>;

std::set<int> ext_pin_upper_zs(const Config& cfg) {
    std::set<int> upper_zs;
    for (const auto& epl : cfg.ext_pin_layer)
        for (const auto& ul : cfg.upper_layers.at(epl)) {
            int z = cfg.routing_layer_index(ul);
            if (z >= 0) upper_zs.insert(z);
        }
    return upper_zs;
}

void classify_edges_for_masking(const RoutingGraph& g, const std::vector<NInfo>& infos,
                                const std::set<int>& upper_zs,
                                const std::function<bool(const std::string&)>& is_true,
                                const CommTrueFn& comm_true,
                                std::set<EP>& masking, std::set<EP>& floating) {
    for (const auto& e : g.edges) {
        const Arr& point = e.first; const Arr& q = e.second;
        if (!upper_zs.count((int)point[2]) && !upper_zs.count((int)q[2])) continue;
        if (!is_true(edge_var_name(point, q))) continue;
        bool any_commodity = false, any_real = false, any_access = false;
        for (const NInfo& ni : infos) {
            if (ni.is_power || ni.is_dummy || !ni.is_ext_pin) continue;
            const int np = (int)ni.net->pins.size();
            for (int i = np - 2; i >= 0; --i) {
                if (i >= ni.comm) continue;
                if (comm_true(ni.cname, i, point, q)) {
                    any_commodity = true;
                    if (i == np - 2) any_access = true; else any_real = true;
                }
            }
        }
        if (any_real) { masking.erase({point, q}); continue; }
        if (any_access) { masking.insert({point, q}); continue; }
        if (!any_commodity) floating.insert({point, q});
    }
}

void propagate_floating_edges(std::set<EP>& masking, std::set<EP>& floating) {
    bool repeat = true;
    while (repeat) {
        std::set<Arr> mask_vertices;
        for (const auto& e : masking) { mask_vertices.insert(e.first); mask_vertices.insert(e.second); }
        repeat = false;
        std::vector<EP> promoted;
        for (const auto& e : floating)
            if (mask_vertices.count(e.first) || mask_vertices.count(e.second)) {
                masking.insert(e); promoted.push_back(e); repeat = true;
            }
        for (const auto& e : promoted) floating.erase(e);
    }
}

void restore_edges_for_connectivity(const std::vector<Net>& nets, const RoutingResult& r,
                                    std::set<EP>& masking) {
    DisjointSet ds;
    for (const auto& e : r.metals) if (!masking.count(e)) ds.unite(e.first, e.second);

    std::map<Arr, std::vector<Arr>> full_adj;
    for (const auto& e : r.metals) { full_adj[e.first].push_back(e.second); full_adj[e.second].push_back(e.first); }

    bool changed = true;
    while (changed) {
        changed = false;
        for (const Net& n : nets) {
            if (n.is_power || n.name == DUMMY_NET) continue;
            std::vector<Arr> pin_points;
            for (const auto& pin : n.pins) {
                if (pin.term == TERM_ACCESS_POINT) continue;
                for (const auto& pt : pin.points) {
                    Arr a{pt.x, pt.y, pt.z};
                    if (ds.contains(a) || full_adj.count(a)) pin_points.push_back(a);
                }
            }
            if (pin_points.empty()) continue;
            for (size_t k = 1; k < pin_points.size(); ++k) {
                Arr base = ds.find(pin_points[0]);
                if (ds.find(pin_points[k]) == base) continue;
                std::map<Arr, Arr> parent;
                std::vector<Arr> queue{pin_points[k]};
                parent[pin_points[k]] = pin_points[k];
                Arr found{}; bool ok = false;
                for (size_t qi = 0; qi < queue.size() && !ok; ++qi) {
                    Arr cur = queue[qi];
                    if (ds.find(cur) == base) { found = cur; ok = true; break; }
                    for (const Arr& nb : full_adj[cur])
                        if (!parent.count(nb)) { parent[nb] = cur; queue.push_back(nb); }
                }
                if (!ok) continue;
                Arr cur = found;
                while (!(cur == pin_points[k])) {
                    Arr prev = parent[cur];
                    EP e = (cur < prev) ? EP{cur, prev} : EP{prev, cur};
                    if (masking.erase(e)) changed = true;
                    ds.unite(cur, prev);
                    cur = prev;
                }
            }
        }
    }
}

void drop_masked_via_enclosures(const std::set<EP>& masking, RoutingResult& r) {
    for (const auto& e : masking) {
        const Arr& point = e.first; const Arr& q = e.second;
        if (!(point[0] == q[0] && point[1] == q[1] && point[2] != q[2])) continue;
        const long x = point[0], y = point[1];
        const long lz = std::min(point[2], q[2]), uz = std::max(point[2], q[2]);
        std::vector<ViaEnc> keep;
        for (const auto& v : r.via_enc) {
            bool drop = (v.x == x && v.y == y) &&
                        ((v.level == 0 && v.z == uz) || (v.level == 1 && v.z == lz));
            if (!drop) keep.push_back(v);
        }
        r.via_enc.swap(keep);
    }
}

void verify_post_mask_connectivity(const std::vector<Net>& nets, const std::set<EP>& masking,
                                   const RoutingResult& r) {
    DisjointSet ds;
    for (const auto& e : r.metals) ds.unite(e.first, e.second);
    std::set<Arr> ever_wired;
    for (const auto& e : r.metals) { ever_wired.insert(e.first); ever_wired.insert(e.second); }
    for (const auto& e : masking) { ever_wired.insert(e.first); ever_wired.insert(e.second); }
    for (const Net& n : nets) {
        if (n.is_power || n.name == DUMMY_NET) continue;
        std::optional<Arr> common;
        bool broken = false, never_routed = false;
        for (const auto& pin : n.pins) {
            if (pin.term == TERM_ACCESS_POINT) continue;
            std::set<Arr> roots;
            bool any_ever_wired = false;
            for (const auto& pt : pin.points) {
                Arr a{pt.x, pt.y, pt.z};
                if (ds.contains(a)) roots.insert(ds.find(a));
                if (ever_wired.count(a)) any_ever_wired = true;
            }
            if (roots.empty()) {
                broken = true;
                if (!any_ever_wired) never_routed = true;
                break;
            }
            if (!common) { common = *roots.begin(); }
            bool has_common = false;
            for (const auto& r2 : roots) if (ds.contains(*common) && ds.find(*common) == r2) { has_common = true; break; }
            if (!has_common) { broken = true; break; }
        }
        if (broken && !never_routed)
            fprintf(stderr, "WARNING: net %s pins are NOT all connected after masking -- "
                    "a masked edge may have been real routing, not access-only slack\n", n.name.c_str());
    }
}

void apply_masking(const Config& cfg, const RoutingGraph& g, const std::vector<Net>& nets,
                   long tol, const std::function<bool(const std::string&)>& is_true, RoutingResult& r) {
    const std::set<int> upper_zs = ext_pin_upper_zs(cfg);
    std::vector<NInfo> infos;
    for (const Net& n : nets) infos.push_back(make_ninfo(n, tol, cfg.x_unit, cfg.y_unit, cfg.power_net, cfg.ground_net));
    CommTrueFn comm_true = [&](const std::string& cn, int i, const Arr& p, const Arr& q) {
        return is_true(comm_edge_var_name(cn, i, p, q));
    };
    std::set<EP> masking, floating;

    classify_edges_for_masking(g, infos, upper_zs, is_true, comm_true, masking, floating);
    propagate_floating_edges(masking, floating);
    restore_edges_for_connectivity(nets, r, masking);

    drop_masked_via_enclosures(masking, r);

    std::vector<std::pair<Arr, Arr>> kept;
    for (const auto& e : r.metals) if (!masking.count(e)) kept.push_back(e);
    r.metals.swap(kept);

    verify_post_mask_connectivity(nets, masking, r);
}

using ObjTerm = std::pair<std::pair<Arr, Arr>, long>;
using ObjSpec = std::vector<std::vector<ObjTerm>>;

ObjSpec build_objective_spec(const Config& cfg, const RoutingGraph& g) {
    const auto& layers = cfg.routing_layers;
    std::map<std::string, int> layer_index;
    for (int i = 0; i < (int)layers.size(); ++i) layer_index[layers[i]] = i;
    std::set<int> ext_pin_zs;
    for (int i = 0; i < (int)layers.size(); ++i)
        if (std::find(cfg.ext_pin_layer.begin(), cfg.ext_pin_layer.end(), layers[i]) != cfg.ext_pin_layer.end())
            ext_pin_zs.insert(i);
    const long gate_contact_y = (long)(cfg.cell_height / 2.0 + cfg.np_offset);
    const std::string order = cfg.option.value("metal_optimization_order", std::string("BIDIRECTION"));
    const bool pin_stretch = cfg.option.value("pin_stretch_aware", false);
    enum Dir { HORIZONTAL = 0, VERTICAL = 1, BIDIRECTION = 2 };

    std::map<Arr, std::vector<Arr>> adjm;
    for (const auto& e : g.edges) { adjm[e.first].push_back(e.second); adjm[e.second].push_back(e.first); }
    std::map<int, std::vector<Arr>> points_by_z;
    for (const auto& p : g.nodes) points_by_z[(int)p[2]].push_back(p);
    auto same_layer_nums = [&](int layer_num) {
        std::vector<int> out;
        for (const auto& nm : cfg.same_height_layers.at(layers[layer_num]))
            if (layer_index.count(nm)) out.push_back(layer_index[nm]);
        return out;
    };

    ObjSpec spec;
    auto z_level = [&](int layer_num) {
        std::vector<ObjTerm> lvl;
        for (int sln : same_layer_nums(layer_num))
            for (const auto& point : points_by_z[sln]) {
                auto it = adjm.find(point);
                if (it == adjm.end()) continue;
                for (const auto& a : it->second)
                    if (a[2] < sln) lvl.push_back({{point, a}, 1});
            }
        if (!lvl.empty()) spec.push_back(std::move(lvl));
    };
    auto metal_levels = [&](int layer_num, Dir dir) {
        const int moving_axis = (dir == HORIZONTAL) ? 0 : 1;
        std::set<long> priorities;
        std::vector<std::tuple<long, long, std::pair<Arr, Arr>>> mv;
        for (int sln : same_layer_nums(layer_num))
            for (const auto& point : points_by_z[sln]) {
                auto it = adjm.find(point);
                if (it == adjm.end()) continue;
                for (const auto& a : it->second) {
                    if (a[2] != sln || a < point) continue;
                    if (dir != BIDIRECTION && point[moving_axis] == a[moving_axis]) continue;
                    long priority = 0;
                    if (pin_stretch && ext_pin_zs.count((int)point[2]) && ext_pin_zs.count((int)a[2]))
                        priority = std::min(std::labs(gate_contact_y - point[1]), std::labs(gate_contact_y - a[1]));
                    const long length = std::labs(point[0] - a[0]) + std::labs(point[1] - a[1]);
                    priorities.insert(priority);
                    mv.emplace_back(length, priority, std::make_pair(point, a));
                }
            }
        for (long target : priorities) {
            std::vector<ObjTerm> lvl;
            for (const auto& t : mv)
                if (std::get<1>(t) == target) lvl.push_back({std::get<2>(t), std::get<0>(t)});
            if (!lvl.empty()) spec.push_back(std::move(lvl));
        }
    };

    std::set<int> done;
    for (int layer_num = (int)layers.size() - 1; layer_num >= 0; --layer_num) {
        if (done.count(layer_num)) continue;
        z_level(layer_num);
        if (order.find("BI") != std::string::npos) metal_levels(layer_num, BIDIRECTION);
        else if (order.find("VER") != std::string::npos) { metal_levels(layer_num, VERTICAL); metal_levels(layer_num, HORIZONTAL); }
        else if (order.find("HOR") != std::string::npos) { metal_levels(layer_num, HORIZONTAL); metal_levels(layer_num, VERTICAL); }
        for (int sln : same_layer_nums(layer_num)) done.insert(sln);
    }
    return spec;
}

void add_objectives(z3::optimize& opt, z3::context& ctx, SmtModel& smt, const ObjSpec& spec,
                    std::vector<z3::optimize::handle>& handles) {
    for (const auto& lvl : spec) {
        z3::expr_vector terms(ctx);
        for (const auto& t : lvl)
            terms.push_back(z3::ite(smt.bool_var(edge_var_name(t.first.first, t.first.second)),
                                    ctx.int_val((int)t.second), ctx.int_val(0)));
        if (!terms.empty()) handles.push_back(opt.minimize(z3::sum(terms)));
    }
}

void apply_z3_params(z3::optimize& opt, z3::context& ctx, const Config& cfg) {
    long t = cfg.option.value("z3_threads", 0L);
    if (t > 0) {
        z3::params p(ctx);
        p.set("sat.threads", (unsigned)t);
        p.set("priority", "lex");
        opt.set(p);
    }
}

}

RoutingResult solve_cell(const Config& cfg, const std::vector<std::vector<long>>& x_points,
                         const std::vector<std::vector<long>>& y_points,
                         const std::vector<std::vector<long>>& ext_pin_y_tracks,
                         const RoutingGraph& g, std::vector<Net>& cnets, PreLayout& pre,
                         const NetOrders& no, const std::string& debug_dump_default) {
    RoutingResult r;
    SmtModel smt;

    std::vector<z3::expr> graph_constraints =
        collect_graph_constraints(smt, cfg, x_points, y_points, ext_pin_y_tracks, g, cnets, pre, no);

    const std::vector<long> schedule = tolerance_schedule(cfg, x_points, y_points);
    const std::string smt2_cfg_path =
        cfg.opt_bool("emit_debug_artifacts") ? debug_dump_default : std::string();
    const char* smt2_dbg = smt2_cfg_path.empty() ? nullptr : smt2_cfg_path.c_str();

    z3::optimize opt(smt.ctx);
    apply_z3_params(opt, smt.ctx, cfg);
    std::unordered_set<unsigned> seen;
    for (const z3::expr& c : graph_constraints)
        if (!c.is_true() && seen.insert(c.id()).second) opt.add(c);
    std::vector<z3::optimize::handle> handles;
    const ObjSpec objspec = build_objective_spec(cfg, g);
    add_objectives(opt, smt.ctx, smt, objspec, handles);
    const long graph_asserts = (long)seen.size();

    for (long T : schedule) {
        smt.tolerance = T;
        std::vector<Net> cnets_T = cnets;
        add_ext_pin_points(cnets_T, cfg, x_points, ext_pin_y_tracks, T);
        add_access_points(cnets_T, cfg, x_points, y_points, T);
        std::vector<z3::expr> net_constraints = collect_net_constraints(smt, cfg, x_points, y_points, g, cnets_T);

        opt.push();
        long net_asserts = 0;
        std::unordered_set<unsigned> net_seen;
        for (const z3::expr& c : net_constraints)
            if (!c.is_true() && !seen.count(c.id()) && net_seen.insert(c.id()).second) { opt.add(c); ++net_asserts; }
        if (smt2_dbg) {
            std::string smt2;
            for (const auto& kv : smt.vars) smt2 += "(declare-fun " + smt2_escape_identifier(kv.first) + " () Bool)\n";
            std::unordered_set<std::string> dbg_seen;
            for (const z3::expr& c : net_constraints) {
                if (c.is_true() || seen.count(c.id())) continue;
                std::string s = smt2_escape_identifier(c.to_string());
                if (dbg_seen.insert(s).second) smt2 += "(assert " + s + ")\n";
            }
            std::ofstream of(smt2_dbg); of << "; (graph asserted once, persistent)\n" << smt2 << "(check-sat)\n";
        }
        long cut_asserts = 0;
        for (const z3::expr& c : smt.add_cuts(cfg, g, cnets_T))
            if (!c.is_true() && net_seen.insert(c.id()).second) { opt.add(c); ++cut_asserts; }
        r.num_constraints = graph_asserts + net_asserts + cut_asserts;

        z3::check_result cr = opt.check();
        if (cr != z3::sat) { opt.pop(); continue; }
        r.sat = true;
        r.tolerance = T;
        z3::model m = opt.get_model();
        for (const auto& e : g.edges) {
            z3::expr v = smt.bool_var(edge_var_name(e.first, e.second));
            if (m.eval(v, true).is_true()) {
                r.metals.push_back(e);
                if (e.first[2] != e.second[2]) ++r.via_count;
                else r.total_length += std::labs(e.first[0] - e.second[0]) + std::labs(e.first[1] - e.second[1]);
            }
        }
        for (const auto& kv : smt.vars) {
            const std::string& nm = kv.first;
            if (nm.rfind("VIA_ENC_", 0) != 0 && nm.rfind("EXT_PIN_", 0) != 0) continue;
            if (!m.eval(kv.second, false).is_true()) continue;
            if (auto ve = try_parse_via_enc(nm)) r.via_enc.push_back(*ve);
            else if (auto ep = try_parse_ext_pin(nm)) r.ext_pins.push_back(*ep);
        }
        if (cfg.option.value("ensure_access_points", false)) {
            apply_masking(cfg, g, cnets_T, T,
                          [&](const std::string& nm) { return m.eval(smt.bool_var(nm), false).is_true(); }, r);
            recompute_route_metrics(r);
        }
        for (const auto& h : handles) {
            z3::expr lo = opt.lower(h);
            r.objective_values.push_back(lo.is_numeral() ? (long)lo.get_numeral_int64() : -1);
        }
        break;
    }
    return r;
}

RoutingResult solve_router(const Config& cfg, const Circuit& circ, const NetOrders& no,
                           const std::string& debug_dump_default) {
    std::set<std::string> valid_tops(cfg.routing_layers.begin(), cfg.routing_layers.end());
    std::vector<std::string> configured =
        cfg.option.value("top_layer_candidates", std::vector<std::string>{"M1", "M2"});
    std::vector<std::string> candidates;
    for (const auto& l : configured)
        if (valid_tops.count(l) && std::find(candidates.begin(), candidates.end(), l) == candidates.end())
            candidates.push_back(l);
    std::string forced = cfg.option.value("top_layer", std::string(""));
    while (!forced.empty() && std::isspace((unsigned char)forced.front())) forced.erase(forced.begin());
    while (!forced.empty() && std::isspace((unsigned char)forced.back())) forced.pop_back();
    if (!forced.empty()) {
        std::vector<std::string> f;
        for (const auto& l : candidates) if (l == forced) f.push_back(l);
        candidates.swap(f);
    }
    if (cfg.option.value("ensure_access_points", false) && !cfg.ext_pin_layer.empty()) {
        int min_top_idx = -1;
        for (const auto& epl : cfg.ext_pin_layer)
            min_top_idx = std::max(min_top_idx, cfg.routing_layer_index(epl));
        min_top_idx += 1;
        std::vector<std::string> f;
        for (const auto& l : candidates) if (cfg.routing_layer_index(l) >= min_top_idx) f.push_back(l);
        candidates.swap(f);
    }
    if (candidates.empty()) candidates.push_back(cfg.routing_layers.back());

    RoutingResult last;
    for (const std::string& top : candidates) {
        Config cfg_t = cfg;
        auto it = std::find(cfg_t.routing_layers.begin(), cfg_t.routing_layers.end(), top);
        if (it != cfg_t.routing_layers.end()) cfg_t.routing_layers.erase(it + 1, cfg_t.routing_layers.end());

        auto xp = get_x_points(cfg_t, (long)no.n_net.size());
        auto yp = get_y_points(cfg_t, cfg_t.opt_bool("allow_below_min_track"));
        auto cnets = compute_net_points(cfg_t, circ, no, yp.y_points);
        if (cfg_t.opt_bool("low_resolution_routing")) {
            std::set<long> valid_x;
            for (const auto& n : cnets)
                if (!(n.is_power || n.name == DUMMY_NET || (!n.is_ext_pin && n.pins.size() < 2)))
                    for (const auto& p : n.pins)
                        for (const auto& pt : p.points) valid_x.insert(pt.x);
            for (auto& layer : xp) {
                std::set<long> inter;
                for (long x : layer) if (valid_x.count(x)) inter.insert(x);
                layer.assign(inter.begin(), inter.end());
            }
        }
        PreLayout pre;
        pre.add_active(cfg_t, no);
        auto rg = build_routing_graph(cfg_t, xp, yp.y_points, yp.ext_pin_y_tracks, pre);
        RoutingResult r = solve_cell(cfg_t, xp, yp.y_points, yp.ext_pin_y_tracks, rg, cnets, pre, no, debug_dump_default);
        r.top_layer = top;
        if (r.sat) return r;
        last = r;
        last.top_layer = top;
    }
    return last;
}

}
