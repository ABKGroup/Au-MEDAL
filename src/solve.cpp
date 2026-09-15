// The solve loop: tolerance/top-layer search over z3, the access-point
// masking post-pass, and result extraction.
#include <cmath>
#include "solve.hpp"

#include "gds.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
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

nlohmann::json routing_result_to_json(const RoutingResult& r) {
    nlohmann::json j;
    j["sat"] = r.sat;
    j["undecided"] = r.undecided;
    j["geometry_unresolved"] = r.geometry_unresolved;
    j["top_layer"] = r.top_layer;
    j["tolerance"] = r.tolerance;
    j["via_count"] = r.via_count;
    j["total_length"] = r.total_length;
    j["num_constraints"] = r.num_constraints;
    j["objective_values"] = r.objective_values;
    j["metals"] = nlohmann::json::array();
    for (const auto& m : r.metals) {
        j["metals"].push_back({
            {"a", {m.first[0], m.first[1], m.first[2]}},
            {"b", {m.second[0], m.second[1], m.second[2]}}
        });
    }
    j["via_enc"] = nlohmann::json::array();
    for (const auto& v : r.via_enc)
        j["via_enc"].push_back({{"x", v.x}, {"y", v.y}, {"z", v.z}, {"dir", v.dir}, {"level", v.level}});
    j["ext_pins"] = nlohmann::json::array();
    for (const auto& p : r.ext_pins)
        j["ext_pins"].push_back({{"net", p.net}, {"x", p.x}, {"y", p.y}, {"z", p.z}});
    return j;
}

RoutingResult routing_result_from_json(const nlohmann::json& j) {
    RoutingResult r;
    r.sat = j.value("sat", false);
    r.undecided = j.value("undecided", false);
    r.geometry_unresolved = j.value("geometry_unresolved", false);
    r.top_layer = j.value("top_layer", std::string());
    r.tolerance = j.value("tolerance", -1L);
    r.via_count = j.value("via_count", 0L);
    r.total_length = j.value("total_length", 0L);
    r.num_constraints = j.value("num_constraints", 0L);
    if (j.contains("objective_values"))
        r.objective_values = j.at("objective_values").get<std::vector<long>>();
    if (j.contains("metals"))
        for (const auto& m : j.at("metals")) {
            std::array<long, 3> a = {m.at("a")[0].get<long>(), m.at("a")[1].get<long>(), m.at("a")[2].get<long>()};
            std::array<long, 3> b = {m.at("b")[0].get<long>(), m.at("b")[1].get<long>(), m.at("b")[2].get<long>()};
            r.metals.push_back({a, b});
        }
    if (j.contains("via_enc"))
        for (const auto& v : j.at("via_enc"))
            r.via_enc.push_back({v.at("x").get<long>(), v.at("y").get<long>(), v.at("z").get<long>(),
                                 v.at("dir").get<int>(), v.at("level").get<int>()});
    if (j.contains("ext_pins"))
        for (const auto& p : j.at("ext_pins"))
            r.ext_pins.push_back({p.at("net").get<std::string>(), p.at("x").get<long>(),
                                  p.at("y").get<long>(), p.at("z").get<long>()});
    return r;
}

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
    append(all, smt.add_adjacent_via_pad_spacing(cfg, g));
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
    append(all, smt.add_pin_grid_witness(cfg, g, cnets));
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
    // Optimize (opt.minimize) proves optimality, not just feasibility -- on a
    // large/hard instance that proof can take far longer than finding a good
    // (non-optimal) routing would. Bound each opt.check() call so a cell
    // that can't be proven optimal quickly still comes back with its best
    // solution so far, rather than hanging indefinitely. 0 = no bound.
    long timeout_ms = cfg.option.value("z3_opt_timeout_ms", 60000L);
    if (t > 0 || timeout_ms > 0) {
        z3::params p(ctx);
        if (t > 0) {
            p.set("sat.threads", (unsigned)t);
            p.set("priority", "lex");
        }
        if (timeout_ms > 0) p.set("timeout", (unsigned)timeout_ms);
        opt.set(p);
    }
}

}


// Cont vias that connect Active to M1 get an extra Active-layer "enclosure"
// square drawn around them at GDS-emission time (emit_bulk_active_contact_
// enclosure in gds.cpp), purely from where the via ends up -- the SAT model
// never reasons about that square's existence. If two such vias each poke
// their square past the edge of the real (fin-count-derived, placement-fixed)
// Active region they sit in, close together but not close enough for the two
// protrusions to touch, the result is an Active notch/spacing (Act.b)
// violation that nothing in the solve stage could have prevented -- found on
// sg13g2_mux2_4, where two Cont vias 40nm apart in Y each protruded 40nm past
// the real Active edge in X, leaving an un-mergeable 40nm sliver gap.
//
// Fix: compute each candidate via's protrusion beyond its Active region
// (using pre.active, which -- like emit_active_regions in gds.cpp -- is
// derived purely from fin counts fixed at placement time, before any SAT
// variable is decided) and forbid choosing two vias together whenever their
// protrusions would leave a sub-min-spacing gap.
std::vector<z3::expr> collect_active_enclosure_conflicts(SmtModel& smt, const Config& cfg,
                                                          const RoutingGraph& g, const PreLayout& pre) {
    std::vector<z3::expr> out;
    if (!cfg.opt_bool("bulk_planar")) return out;
    const auto& layers = cfg.routing_layers;
    const int active_z = cfg.routing_layer_index("Active");
    const int m1_z = cfg.routing_layer_index("M1");
    if (active_z < 0 || m1_z < 0) return out;

    const double half = cfg.width("Cont") / 2.0 + (double)cfg.opt_long("active_contact_enclosure");
    // NOT design_rules.spacing.S2S.Active.Active -- this router's own model
    // deliberately keeps that at 0 (it never routes free Active wire
    // segments that would need self-spacing). This needs the real IHP
    // SG13G2 DRC deck value for rule Act.b ("Min. Activ space or notch:
    // 0.21 um"), which lives nowhere else in this config.
    const double min_spacing = (double)cfg.opt_long("active_min_notch_spacing");
    if (half <= 0 || min_spacing <= 0) return out;

    using D4 = std::array<double, 4>;  // {x0, y0, x1, y1}

    struct Candidate {
        std::array<long, 3> a, b;
        double cx, cy;
        std::vector<D4> protrusions;
    };
    std::vector<Candidate> cands;

    for (const auto& e : g.edges) {
        const auto& p = e.first;
        const auto& q = e.second;
        if (p[2] == q[2]) continue;
        const auto& lo = (p[2] < q[2]) ? p : q;
        const auto& hi = (p[2] < q[2]) ? q : p;
        if (lo[2] != active_z || hi[2] != m1_z) continue;
        if (lo[0] != hi[0] || lo[1] != hi[1]) continue;  // vias sit at one (x,y)

        Candidate c;
        c.a = p; c.b = q;
        c.cx = (double)lo[0]; c.cy = (double)lo[1];
        const double sx0 = c.cx - half, sy0 = c.cy - half, sx1 = c.cx + half, sy1 = c.cy + half;

        // Which real Active geometry (placement-fixed) does this via sit
        // over? Its own center isn't always inside a single pre.active
        // rect -- e.g. a via at an odd (non-transistor-gate) column, on
        // shared diffusion between two even columns, has no pre.active
        // entry of its own even though real Active does cover it there.
        // So: take the union (bounding extent) of every pre.active rect
        // that overlaps the enclosure square at all, not just the one
        // (if any) that contains the via's exact center.
        bool any_region = false;
        double rx0 = sx0, ry0 = sy0, rx1 = sx1, ry1 = sy1;
        for (const auto& r : pre.active) {
            if (r[0] >= sx1 || r[2] <= sx0 || r[1] >= sy1 || r[3] <= sy0) continue;  // no overlap
            if (!any_region) { rx0 = r[0]; ry0 = r[1]; rx1 = r[2]; ry1 = r[3]; any_region = true; }
            else {
                rx0 = std::min(rx0, r[0]); ry0 = std::min(ry0, r[1]);
                rx1 = std::max(rx1, r[2]); ry1 = std::max(ry1, r[3]);
            }
        }
        if (!any_region) {
            // No pre.active rect overlaps this enclosure square at all --
            // an isolated via touching neither region it sits between. The
            // whole square is exposed, not just a strip off some region's
            // edge (same fix as find_active_geometry_conflicts()).
            c.protrusions.push_back({sx0, sy0, sx1, sy1});
        } else {
            if (rx0 - sx0 > 0) c.protrusions.push_back({sx0, sy0, rx0, sy1});
            if (sx1 - rx1 > 0) c.protrusions.push_back({rx1, sy0, sx1, sy1});
            if (ry0 - sy0 > 0) c.protrusions.push_back({sx0, sy0, sx1, ry0});
            if (sy1 - ry1 > 0) c.protrusions.push_back({sx0, ry1, sx1, sy1});
        }

        // Active also runs as a full-width band around each power rail
        // (emit_power_rails, gds.cpp): y in [rail-150, rail+150] for
        // rail in {0, cell_height}, spanning the whole cell. A protrusion
        // that falls entirely inside one of those bands is already covered
        // by real geometry, not a bare sliver -- drop it. Same for a
        // protrusion that lands inside some OTHER pre.active rect (e.g. the
        // neighboring column's Active) -- it isn't "its own" region, but
        // it's still real, already-there geometry, not exposed silicon.
        const double ch = (double)cfg.cell_height;
        const double rail_half = 150.0;
        auto is_covered = [&](const D4& p) {
            for (double rail : {0.0, ch})
                if (p[1] >= rail - rail_half && p[3] <= rail + rail_half) return true;
            for (const auto& r : pre.active)
                if (p[0] >= r[0] && p[2] <= r[2] && p[1] >= r[1] && p[3] <= r[3]) return true;
            return false;
        };
        c.protrusions.erase(std::remove_if(c.protrusions.begin(), c.protrusions.end(), is_covered),
                            c.protrusions.end());

        if (!c.protrusions.empty()) cands.push_back(std::move(c));
    }

    auto gap_1d = [](double a0, double a1, double b0, double b1) {
        if (a1 <= b0) return b0 - a1;
        if (b1 <= a0) return a0 - b1;
        return -1.0;  // overlap
    };

    for (size_t i = 0; i < cands.size(); ++i) {
        for (size_t j = i + 1; j < cands.size(); ++j) {
            if (std::abs(cands[i].cx - cands[j].cx) > 4 * half + min_spacing &&
                std::abs(cands[i].cy - cands[j].cy) > 4 * half + min_spacing)
                continue;
            bool conflict = false;
            for (const auto& p1 : cands[i].protrusions) {
                for (const auto& p2 : cands[j].protrusions) {
                    const double xgap = gap_1d(p1[0], p1[2], p2[0], p2[2]);
                    const double ygap = gap_1d(p1[1], p1[3], p2[1], p2[3]);
                    const bool x_overlaps = (xgap < 0);
                    const bool y_overlaps = (ygap < 0);
                    if (x_overlaps && ygap > 0 && ygap < min_spacing) { conflict = true; break; }
                    if (y_overlaps && xgap > 0 && xgap < min_spacing) { conflict = true; break; }
                }
                if (conflict) break;
            }
            if (!conflict) continue;
            z3::expr v1a = smt.bool_var(edge_var_name(cands[i].a, cands[i].b));
            z3::expr v2a = smt.bool_var(edge_var_name(cands[j].a, cands[j].b));
            out.push_back(!(v1a && v2a));
        }
    }
    return out;
}

RoutingResult solve_cell(const Config& cfg, const std::vector<std::vector<long>>& x_points,
                         const std::vector<std::vector<long>>& y_points,
                         const std::vector<std::vector<long>>& ext_pin_y_tracks,
                         const RoutingGraph& g, std::vector<Net>& cnets, PreLayout& pre,
                         const NetOrders& no, const std::string& debug_dump_default,
                         const std::vector<std::pair<std::string, std::string>>& forbidden_pairs) {
    RoutingResult r;
    SmtModel smt;

    std::vector<z3::expr> graph_constraints =
        collect_graph_constraints(smt, cfg, x_points, y_points, ext_pin_y_tracks, g, cnets, pre, no);
    {
        std::vector<z3::expr> enc_conflicts = collect_active_enclosure_conflicts(smt, cfg, g, pre);
        for (z3::expr& c : enc_conflicts) graph_constraints.push_back(std::move(c));
    }
    for (const auto& fp : forbidden_pairs)
        graph_constraints.push_back(!(smt.bool_var(fp.first) && smt.bool_var(fp.second)));

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
        smt.pin_shift = site_pad_shift(cfg, no);
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

        // Every tolerance is optimised. A layout that merely satisfies the
        // rules minimises nothing, and shipping one is a decision about the
        // deliverable, not a way to get under a timeout.
        z3::check_result cr = opt.check();

        bool have_model = false;
        z3::model m(smt.ctx);  // placeholder, reassigned below once we know which solver has a model
        if (cr == z3::unknown && T == schedule.front()) r.undecided = true;
        if (cr == z3::sat) {
            m = opt.get_model();
            have_model = true;
        } else if (cr == z3::unknown) {
            // The bound was hit before optimality could be proven, but the
            // optimizer still holds the best model it reached under the same
            // objectives. That incumbent is an optimised layout whose
            // optimality is unproven, not an objective-free one, so take it
            // rather than discarding the whole search. Measured on z3 4.15.4:
            // an unknown verdict still yields a complete satisfying model.
            try {
                z3::model cand = opt.get_model();
                if (cand.size() > 0) { m = cand; have_model = true; }
            } catch (const z3::exception&) {}
            if (!have_model) r.undecided = true;
        }
        if (!have_model) { opt.pop(); continue; }

        r.sat = true;
        r.tolerance = T;
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
        {
            // A plain-fallback model (opt timed out, so no wirelength
            // objective ran) freely sets redundant edges true. A closed
            // one-track ring's interior slit is an M1.b/M2.b notch that no
            // node-anchored spacing family can express, so drop cycle legs,
            // dangling wire stubs, and vias whose upper side connects to
            // nothing, before detectors or emission see the solution. A
            // removed edge is always redundant for connectivity, checked
            // per edge against the full remaining graph.
            using Node = std::array<long, 3>;
            std::set<Node> pin_pts;
            for (const auto& n : cnets_T)
                for (const auto& pin : n.pins)
                    for (const auto& pt : pin.points) pin_pts.insert({pt.x, pt.y, pt.z});
            // One true pin grid witness edge per port net survives the cleanup, or the emitter finds no grid metal.
            std::set<std::pair<Node, Node>> keep;
            {
                std::set<std::string> held;
                for (const auto& w : smt.pin_grid_lits) {
                    if (held.count(w.net) || !m.eval(w.lit, false).is_true()) continue;
                    held.insert(w.net);
                    keep.insert({w.a, w.b});
                    keep.insert({w.b, w.a});
                }
            }
            auto kept = [&](size_t i) { return keep.count({r.metals[i].first, r.metals[i].second}) > 0; };
            auto degree_map = [&]() {
                std::map<Node, int> d;
                for (const auto& e : r.metals) { d[e.first]++; d[e.second]++; }
                return d;
            };
            auto connected_without = [&](size_t skip_idx) {
                const Node& s = r.metals[skip_idx].first;
                const Node& t = r.metals[skip_idx].second;
                std::map<Node, std::vector<Node>> adj;
                for (size_t i = 0; i < r.metals.size(); ++i) {
                    if (i == skip_idx) continue;
                    adj[r.metals[i].first].push_back(r.metals[i].second);
                    adj[r.metals[i].second].push_back(r.metals[i].first);
                }
                std::set<Node> seen{s};
                std::vector<Node> q{s};
                for (size_t qi = 0; qi < q.size(); ++qi) {
                    if (q[qi] == t) return true;
                    for (const auto& nx : adj[q[qi]])
                        if (seen.insert(nx).second) q.push_back(nx);
                }
                return false;
            };
            bool changed = true;
            while (changed) {
                changed = false;
                for (size_t i = 0; i < r.metals.size(); ++i) {
                    if (r.metals[i].first[2] != r.metals[i].second[2]) continue;
                    if (!kept(i) && connected_without(i)) {
                        r.metals.erase(r.metals.begin() + (long)i);
                        changed = true;
                        break;
                    }
                }
            }
            changed = true;
            while (changed) {
                changed = false;
                auto deg = degree_map();
                for (size_t i = 0; i < r.metals.size(); ++i) {
                    const auto& a = r.metals[i].first;
                    const auto& b = r.metals[i].second;
                    if (a[2] == b[2]) {
                        // dangling wire stub off any non-pin endpoint
                        if (!kept(i) && ((deg[a] == 1 && !pin_pts.count(a)) || (deg[b] == 1 && !pin_pts.count(b)))) {
                            r.metals.erase(r.metals.begin() + (long)i);
                            changed = true;
                            break;
                        }
                    } else {
                        // via whose upper side reaches nothing and is not a
                        // pin. Gate-side vias stay: the poly column itself
                        // carries connectivity the edge list cannot see.
                        const Node& lo = (a[2] < b[2]) ? a : b;
                        const Node& hi = (a[2] < b[2]) ? b : a;
                        const std::string& lo_layer = cfg.routing_layers[lo[2]];
                        if (lo_layer == cfg.gate_contact_layer) continue;
                        if (!kept(i) && deg[hi] == 1 && !pin_pts.count(hi)) {
                            r.metals.erase(r.metals.begin() + (long)i);
                            changed = true;
                            break;
                        }
                    }
                }
            }
            std::set<std::pair<long, long>> via_xy;
            for (const auto& e : r.metals)
                if (e.first[2] != e.second[2]) via_xy.insert({e.first[0], e.first[1]});
            std::vector<ViaEnc> kept_enc;
            for (const auto& v : r.via_enc)
                if (via_xy.count({v.x, v.y})) kept_enc.push_back(v);
            r.via_enc.swap(kept_enc);
            recompute_route_metrics(r);
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


// Post-solve counterpart to collect_active_enclosure_conflicts(): that one
// only sees Cont-Active-M1 via *candidates* from the routing graph, computed
// before solving. But two other GDS-emission-time-only sources also add
// unconstrained Active geometry around a *chosen* solution -- via_enc
// entries (VIA_ENC_* variables, gds.cpp's emit_via_enclosure_rects) and the
// bulk-contact-enclosure squares themselves, now anchored to the actual
// via positions in a found model rather than every candidate. Recomputing
// both from a solved RoutingResult and checking them against each other and
// against pre.active catches what the upfront candidate pass can miss.
struct ActiveGeomVar { double x0, y0, x1, y1; std::string var; long px, py; };

std::vector<std::pair<std::string, std::string>> find_active_geometry_conflicts(
        const Config& cfg, const PreLayout& pre, const RoutingResult& r) {
    std::vector<std::pair<std::string, std::string>> out;
    if (!cfg.opt_bool("bulk_planar")) return out;
    const int active_z = cfg.routing_layer_index("Active");
    if (active_z < 0) return out;
    const double min_spacing = (double)cfg.opt_long("active_min_notch_spacing");
    if (min_spacing <= 0) return out;
    const double ch = (double)cfg.cell_height;

    std::vector<ActiveGeomVar> items;

    // Source 1: bulk Cont-Active-M1 enclosure squares around chosen vias.
    const double bulk_half = cfg.width("Cont") / 2.0 + (double)cfg.opt_long("active_contact_enclosure");
    if (bulk_half > 0) {
        for (const auto& e : r.metals) {
            const auto& a = e.first; const auto& b = e.second;
            if (a[2] == b[2]) continue;
            const auto& lo = (a[2] < b[2]) ? a : b;
            const auto& hi = (a[2] < b[2]) ? b : a;
            if (cfg.routing_layers[lo[2]] != "Active" || cfg.routing_layers[hi[2]] != "M1") continue;
            if (lo[0] != hi[0] || lo[1] != hi[1]) continue;
            const double cx = (double)lo[0], cy = (double)lo[1];
            items.push_back({cx - bulk_half, cy - bulk_half, cx + bulk_half, cy + bulk_half,
                             edge_var_name(a, b), lo[0], lo[1]});
        }
    }

    // Source 2: via_enc-derived Active enclosure rects (mirrors gds.cpp's
    // emit_via_enclosure_rects for layer == "Active").
    for (const auto& v : r.via_enc) {
        if (v.z != active_z) continue;
        const auto& via = (v.level == 0) ? cfg.lower_via.at("Active") : cfg.upper_via.at("Active");
        if (!via.has_value() || via->empty()) continue;
        const long metal_width = (v.y % (long)ch == 0 && cfg.is_power_layer("Active"))
                                      ? cfg.power_width("Active") : cfg.width("Active");
        const long metal_ex = cfg.rules.at("extension").at("Active").get<long>();
        const long via_width = cfg.width(*via);
        long enc = 0;
        const auto& enclosure_rules = cfg.rules.at("enclosure");
        if (enclosure_rules.contains(*via)) {
            const auto& via_rules = enclosure_rules.at(*via);
            if (via_rules.contains("Active")) enc = via_rules.at("Active").get<long>();
        }
        long xe, ye, xmw, ymw;
        if (v.dir == 0) { xe = enc; ye = 0; xmw = metal_ex; ymw = metal_width; }
        else { xe = 0; ye = enc; xmw = metal_width; ymw = metal_ex; }
        const double lx = (xe != 0) ? v.x - (via_width / 2.0 + xe) : v.x - xmw / 2.0;
        const double ux = (xe != 0) ? v.x + (via_width / 2.0 + xe) : v.x + xmw / 2.0;
        const double ly = (ye != 0) ? v.y - (via_width / 2.0 + ye) : v.y - ymw / 2.0;
        const double uy = (ye != 0) ? v.y + (via_width / 2.0 + ye) : v.y + ymw / 2.0;
        std::string dir_s = (v.dir == 0) ? "HOR" : "VER";
        std::string lvl_s = (v.level == 0) ? "L" : "U";
        std::string var = "VIA_ENC_" + dir_s + "_" + lvl_s + "_G_x" + std::to_string(v.x) +
                          "y" + std::to_string(v.y) + "z" + std::to_string(v.z);
        items.push_back({lx, ly, ux, uy, var, v.x, v.y});
    }

    if (items.empty()) return out;

    using D4 = std::array<double, 4>;
    auto protrusions_of = [&](size_t self_idx) {
        const ActiveGeomVar& it = items[self_idx];
        std::vector<D4> pr;
        bool any = false;
        double rx0 = it.x0, ry0 = it.y0, rx1 = it.x1, ry1 = it.y1;
        for (const auto& reg : pre.active) {
            if (reg[0] >= it.x1 || reg[2] <= it.x0 || reg[1] >= it.y1 || reg[3] <= it.y0) continue;
            if (!any) { rx0 = reg[0]; ry0 = reg[1]; rx1 = reg[2]; ry1 = reg[3]; any = true; }
            else {
                rx0 = std::min(rx0, reg[0]); ry0 = std::min(ry0, reg[1]);
                rx1 = std::max(rx1, reg[2]); ry1 = std::max(ry1, reg[3]);
            }
        }
        for (size_t k = 0; k < items.size(); ++k) {
            if (k == self_idx) continue;
            if (items[k].px == it.px && items[k].py == it.py) continue;
            const double reg0 = items[k].x0, reg1 = items[k].y0, reg2 = items[k].x1, reg3 = items[k].y1;
            if (reg0 >= it.x1 || reg2 <= it.x0 || reg1 >= it.y1 || reg3 <= it.y0) continue;
            if (!any) { rx0 = reg0; ry0 = reg1; rx1 = reg2; ry1 = reg3; any = true; }
            else {
                rx0 = std::min(rx0, reg0); ry0 = std::min(ry0, reg1);
                rx1 = std::max(rx1, reg2); ry1 = std::max(ry1, reg3);
            }
        }
        if (!any) {
            // No pre.active rect overlaps this enclosure square at all --
            // e.g. an isolated Cont via sitting between two real Active
            // pieces (or a rail), touching neither. The whole square is
            // exposed, not just a strip off some region's edge.
            pr.push_back({it.x0, it.y0, it.x1, it.y1});
        } else {
            if (rx0 - it.x0 > 0) pr.push_back({it.x0, it.y0, rx0, it.y1});
            if (it.x1 - rx1 > 0) pr.push_back({rx1, it.y0, it.x1, it.y1});
            if (ry0 - it.y0 > 0) pr.push_back({it.x0, it.y0, it.x1, ry0});
            if (it.y1 - ry1 > 0) pr.push_back({it.x0, ry1, it.x1, it.y1});
        }
        const double rail_half = 150.0;
        auto covered = [&](const D4& p) {
            for (double rail : {0.0, ch})
                if (p[1] >= rail - rail_half && p[3] <= rail + rail_half) return true;
            for (const auto& reg : pre.active)
                if (p[0] >= reg[0] && p[2] <= reg[2] && p[1] >= reg[1] && p[3] <= reg[3]) return true;
            for (size_t k = 0; k < items.size(); ++k) {
                if (k == self_idx) continue;
                if (items[k].px == it.px && items[k].py == it.py) continue;
                if (p[0] >= items[k].x0 && p[2] <= items[k].x1 && p[1] >= items[k].y0 && p[3] <= items[k].y1) return true;
            }
            return false;
        };
        pr.erase(std::remove_if(pr.begin(), pr.end(), covered), pr.end());
        return pr;
    };

    auto gap_1d = [](double a0, double a1, double b0, double b1) {
        if (a1 <= b0) return b0 - a1;
        if (b1 <= a0) return a0 - b1;
        return -1.0;
    };

    std::vector<std::vector<D4>> all_pr(items.size());
    for (size_t i = 0; i < items.size(); ++i) all_pr[i] = protrusions_of(i);

    std::set<std::pair<std::string, std::string>> seen_pairs;
    for (size_t i = 0; i < items.size(); ++i) {
        if (all_pr[i].empty()) continue;
        for (size_t j = i + 1; j < items.size(); ++j) {
            if (all_pr[j].empty()) continue;
            if (items[i].var == items[j].var) continue;
            if (items[i].px == items[j].px && items[i].py == items[j].py) continue;
            bool conflict = false;
            for (const auto& p1 : all_pr[i]) {
                for (const auto& p2 : all_pr[j]) {
                    const double xgap = gap_1d(p1[0], p1[2], p2[0], p2[2]);
                    const double ygap = gap_1d(p1[1], p1[3], p2[1], p2[3]);
                    if (xgap < 0 && ygap > 0 && ygap < min_spacing) { conflict = true; break; }
                    if (ygap < 0 && xgap > 0 && xgap < min_spacing) { conflict = true; break; }
                    if (xgap > 0 && ygap > 0 && xgap * xgap + ygap * ygap < min_spacing * min_spacing) { conflict = true; break; }
                }
                if (conflict) break;
            }
            if (!conflict) continue;
            auto key = (items[i].var < items[j].var) ? std::make_pair(items[i].var, items[j].var)
                                                      : std::make_pair(items[j].var, items[i].var);
            seen_pairs.insert(key);
            out.push_back(key);
        }
    }

    // A lone via's protrusion can violate spacing with no second via
    // involved: if it only partially bridges a gap between two real Active
    // pieces (or a rail), the *remaining* sub-gap on either side of it can
    // itself be sub-min-spacing. Check every protrusion against the nearest
    // edge of every pre.active rect / rail band it doesn't already overlap;
    // route the exclusion back as (var, var) so the caller forbids it
    // outright rather than needing a partner.
    for (size_t i = 0; i < items.size(); ++i) {
        if (all_pr[i].empty()) continue;
        bool self_conflict = false;
        for (const auto& p1 : all_pr[i]) {
            for (const auto& reg : pre.active) {
                const double xgap = gap_1d(p1[0], p1[2], reg[0], reg[2]);
                const double ygap = gap_1d(p1[1], p1[3], reg[1], reg[3]);
                // Touching (gap == 0) is fine -- the shapes are continuous.
                // Only a *positive* gap under the minimum is a real notch.
                if (xgap < 0 && ygap > 0 && ygap < min_spacing) { self_conflict = true; break; }
                if (ygap < 0 && xgap > 0 && xgap < min_spacing) { self_conflict = true; break; }
                if (xgap > 0 && ygap > 0 && xgap * xgap + ygap * ygap < min_spacing * min_spacing) { self_conflict = true; break; }
            }
            if (self_conflict) break;
            for (double rail : {0.0, ch}) {
                const double ygap = gap_1d(p1[1], p1[3], rail - 150.0, rail + 150.0);
                if (ygap > 0 && ygap < min_spacing) { self_conflict = true; break; }
            }
            if (self_conflict) break;
        }
        if (self_conflict) {
            out.push_back({items[i].var, items[i].var});
        }
    }
    return out;
}


// Same architecture gap as find_active_geometry_conflicts(), for the Gate
// layer instead of Active: Gate-Cont via_enc rects are added at
// GDS-emission time (emit_via_enclosure_rects) with no SAT awareness, so
// two of them (or one plus the real gate poly column) can end up too close
// -- a Gat.b "space or notch" violation (real IHP rule: 0.18um; not the
// router's own S2S.Gate.Gate=440, which governs something else and is why
// this needs a value of its own, same story as Active).
//
// Unlike Active, Gate's "real, placement-fixed" geometry doesn't need fin
// counts: emit_gates() (gds.cpp) draws one full-cell-height strip per gate
// column at a uniform pitch, deterministic from cfg/no alone. Build the
// identical list here.
std::vector<std::pair<std::string, std::string>> find_gate_geometry_conflicts(
        const Config& cfg, const NetOrders& no, const PreLayout& pre, const RoutingResult& r) {
    std::vector<std::pair<std::string, std::string>> out;
    if (!cfg.opt_bool("bulk_planar")) return out;
    const int gate_z = cfg.routing_layer_index("Gate");
    if (gate_z < 0) return out;
    const double min_spacing = (double)cfg.opt_long("gate_min_notch_spacing");
    if (min_spacing <= 0) return out;
    const double ch = (double)cfg.cell_height;

    const long num_poly = ((long)no.p_net.size() + 1) / 2;
    const double x_off = cfg.x_offset;
    const double gate_pitch = cfg.pitch.at("Gate");
    const double gate_w = cfg.width("Gate");
    const long begin = 1, end = std::max(1L, num_poly - 1);
    std::vector<std::array<double, 4>> fixed_gate;
    for (long x = begin; x < end; ++x) {
        const double cx = x_off + x * gate_pitch;
        fixed_gate.push_back({cx - gate_w / 2.0, 0.0, cx + gate_w / 2.0, ch});
    }

    struct GeomVar { double x0, y0, x1, y1; std::string var; long px, py; };
    std::vector<GeomVar> items;

    for (const auto& e : r.metals) {
        const auto& a = e.first; const auto& b = e.second;
        if (a[2] == b[2]) continue;
        const auto& lo = (a[2] < b[2]) ? a : b;
        const auto& hi = (a[2] < b[2]) ? b : a;
        if (cfg.routing_layers[lo[2]] != "Gate" || cfg.routing_layers[hi[2]] != "M1") continue;
        if (lo[0] != hi[0] || lo[1] != hi[1]) continue;
        const double half = cfg.width("Cont") / 2.0 + (double)cfg.opt_long("active_contact_enclosure");
        if (half <= 0) continue;
        const double cx = (double)lo[0], cy = (double)lo[1];
        items.push_back({cx - half, cy - half, cx + half, cy + half, edge_var_name(a, b), lo[0], lo[1]});
    }

    for (const auto& v : r.via_enc) {
        if (v.z != gate_z) continue;
        const auto& via = (v.level == 0) ? cfg.lower_via.at("Gate") : cfg.upper_via.at("Gate");
        if (!via.has_value() || via->empty()) continue;
        const long metal_width = cfg.width("Gate");
        const long metal_ex = cfg.rules.at("extension").at("Gate").get<long>();
        const long via_width = cfg.width(*via);
        long enc = 0;
        const auto& enclosure_rules = cfg.rules.at("enclosure");
        if (enclosure_rules.contains(*via)) {
            const auto& via_rules = enclosure_rules.at(*via);
            if (via_rules.contains("Gate")) enc = via_rules.at("Gate").get<long>();
        }
        long xe, ye, xmw, ymw;
        if (v.dir == 0) { xe = enc; ye = 0; xmw = metal_ex; ymw = metal_width; }
        else { xe = 0; ye = enc; xmw = metal_width; ymw = metal_ex; }
        const double lx = (xe != 0) ? v.x - (via_width / 2.0 + xe) : v.x - xmw / 2.0;
        const double ux = (xe != 0) ? v.x + (via_width / 2.0 + xe) : v.x + xmw / 2.0;
        const double ly = (ye != 0) ? v.y - (via_width / 2.0 + ye) : v.y - ymw / 2.0;
        const double uy = (ye != 0) ? v.y + (via_width / 2.0 + ye) : v.y + ymw / 2.0;
        std::string dir_s = (v.dir == 0) ? "HOR" : "VER";
        std::string lvl_s = (v.level == 0) ? "L" : "U";
        std::string var = "VIA_ENC_" + dir_s + "_" + lvl_s + "_G_x" + std::to_string(v.x) +
                          "y" + std::to_string(v.y) + "z" + std::to_string(v.z);
        items.push_back({lx, ly, ux, uy, var, v.x, v.y});
    }
    {
        std::vector<std::pair<std::array<long, 2>, std::array<long, 2>>> gate_edges_2d;
        std::vector<std::pair<std::array<long, 3>, std::array<long, 3>>> gate_edges_3d;
        for (const auto& e : r.metals) {
            const auto& a = e.first; const auto& b = e.second;
            if (a[2] != b[2] || a[2] != gate_z) continue;
            gate_edges_2d.push_back({{a[0], a[1]}, {b[0], b[1]}});
            gate_edges_3d.push_back(e);
        }
        for (const auto& span : merge_collinear(gate_edges_2d)) {
            std::string rep_var;
            const bool vertical = span.first[0] == span.second[0];
            for (const auto& e : gate_edges_3d) {
                const auto& a = e.first; const auto& b = e.second;
                if (vertical) {
                    if (a[0] != span.first[0] || b[0] != span.first[0]) continue;
                    if (std::min(a[1], b[1]) < span.first[1] || std::max(a[1], b[1]) > span.second[1]) continue;
                } else {
                    if (a[1] != span.first[1] || b[1] != span.first[1]) continue;
                    if (std::min(a[0], b[0]) < span.first[0] || std::max(a[0], b[0]) > span.second[0]) continue;
                }
                rep_var = edge_var_name(a, b);
                break;
            }
            if (rep_var.empty()) continue;
            Rect sq = get_square(cfg, span.first[0], span.first[1], span.second[0], span.second[1], "Gate", pre);
            items.push_back({sq[0], sq[1], sq[2], sq[3], rep_var, span.first[0], span.first[1]});
        }
    }
    // Gate contacts draw a Cnt.d landing pad on poly (emit_metals), wider
    // than anything the sources above produce. Model it so the detector sees
    // what the deck will measure.
    {
        const int m1_z_pad = cfg.routing_layer_index("M1");
        const double pad = cfg.width("Cont") / 2.0 + 70.0;
        for (const auto& e : r.metals) {
            const auto& a = e.first;
            const auto& b = e.second;
            if (a[2] == b[2]) continue;
            const int lz = (int)std::min(a[2], b[2]), uz = (int)std::max(a[2], b[2]);
            if (cfg.routing_layers[lz] != cfg.gate_contact_layer || uz != m1_z_pad) continue;
            items.push_back({a[0] - pad, a[1] - pad, a[0] + pad, a[1] + pad,
                             edge_var_name(a, b), a[0], a[1]});
        }
    }

    if (items.empty()) return out;

    using D4 = std::array<double, 4>;
    auto gap_1d = [](double a0, double a1, double b0, double b1) {
        if (a1 <= b0) return b0 - a1;
        if (b1 <= a0) return a0 - b1;
        return -1.0;
    };
    auto protrusions_of = [&](size_t self_idx) {
        const GeomVar& it = items[self_idx];
        std::vector<D4> pr;
        bool any = false;
        double rx0 = it.x0, ry0 = it.y0, rx1 = it.x1, ry1 = it.y1;
        for (const auto& reg : fixed_gate) {
            if (reg[0] >= it.x1 || reg[2] <= it.x0 || reg[1] >= it.y1 || reg[3] <= it.y0) continue;
            if (!any) { rx0 = reg[0]; ry0 = reg[1]; rx1 = reg[2]; ry1 = reg[3]; any = true; }
            else {
                rx0 = std::min(rx0, reg[0]); ry0 = std::min(ry0, reg[1]);
                rx1 = std::max(rx1, reg[2]); ry1 = std::max(ry1, reg[3]);
            }
        }
        for (size_t k = 0; k < items.size(); ++k) {
            if (k == self_idx) continue;
            if (items[k].px == it.px && items[k].py == it.py) continue;
            const double reg0 = items[k].x0, reg1 = items[k].y0, reg2 = items[k].x1, reg3 = items[k].y1;
            if (reg0 >= it.x1 || reg2 <= it.x0 || reg1 >= it.y1 || reg3 <= it.y0) continue;
            if (!any) { rx0 = reg0; ry0 = reg1; rx1 = reg2; ry1 = reg3; any = true; }
            else {
                rx0 = std::min(rx0, reg0); ry0 = std::min(ry0, reg1);
                rx1 = std::max(rx1, reg2); ry1 = std::max(ry1, reg3);
            }
        }
        if (!any) { pr.push_back({it.x0, it.y0, it.x1, it.y1}); return pr; }
        if (rx0 - it.x0 > 0) pr.push_back({it.x0, it.y0, rx0, it.y1});
        if (it.x1 - rx1 > 0) pr.push_back({rx1, it.y0, it.x1, it.y1});
        if (ry0 - it.y0 > 0) pr.push_back({it.x0, it.y0, it.x1, ry0});
        if (it.y1 - ry1 > 0) pr.push_back({it.x0, ry1, it.x1, it.y1});
        auto covered = [&](const D4& p) {
            for (const auto& reg : fixed_gate)
                if (p[0] >= reg[0] && p[2] <= reg[2] && p[1] >= reg[1] && p[3] <= reg[3]) return true;
            for (size_t k = 0; k < items.size(); ++k) {
                if (k == self_idx) continue;
                if (items[k].px == it.px && items[k].py == it.py) continue;
                if (p[0] >= items[k].x0 && p[2] <= items[k].x1 && p[1] >= items[k].y0 && p[3] <= items[k].y1) return true;
            }
            return false;
        };
        pr.erase(std::remove_if(pr.begin(), pr.end(), covered), pr.end());
        return pr;
    };

    std::vector<std::vector<D4>> all_pr(items.size());
    for (size_t i = 0; i < items.size(); ++i) all_pr[i] = protrusions_of(i);

    std::set<std::pair<std::string, std::string>> seen_pairs;
    for (size_t i = 0; i < items.size(); ++i) {
        if (all_pr[i].empty()) continue;
        for (size_t j = i + 1; j < items.size(); ++j) {
            if (all_pr[j].empty()) continue;
            if (items[i].var == items[j].var) continue;
            if (items[i].px == items[j].px && items[i].py == items[j].py) continue;
            bool conflict = false;
            for (const auto& p1 : all_pr[i]) {
                for (const auto& p2 : all_pr[j]) {
                    const double xgap = gap_1d(p1[0], p1[2], p2[0], p2[2]);
                    const double ygap = gap_1d(p1[1], p1[3], p2[1], p2[3]);
                    if (xgap < 0 && ygap > 0 && ygap < min_spacing) { conflict = true; break; }
                    if (ygap < 0 && xgap > 0 && xgap < min_spacing) { conflict = true; break; }
                    if (xgap > 0 && ygap > 0 && xgap * xgap + ygap * ygap < min_spacing * min_spacing) { conflict = true; break; }
                }
                if (conflict) break;
            }
            if (!conflict) continue;
            auto key = (items[i].var < items[j].var) ? std::make_pair(items[i].var, items[j].var)
                                                      : std::make_pair(items[j].var, items[i].var);
            seen_pairs.insert(key);
            out.push_back(key);
        }
    }
    for (size_t i = 0; i < items.size(); ++i) {
        if (all_pr[i].empty()) continue;
        bool self_conflict = false;
        for (const auto& p1 : all_pr[i]) {
            for (const auto& reg : fixed_gate) {
                const double xgap = gap_1d(p1[0], p1[2], reg[0], reg[2]);
                const double ygap = gap_1d(p1[1], p1[3], reg[1], reg[3]);
                if (xgap < 0 && ygap > 0 && ygap < min_spacing) { self_conflict = true; break; }
                if (ygap < 0 && xgap > 0 && xgap < min_spacing) { self_conflict = true; break; }
                if (xgap > 0 && ygap > 0 && xgap * xgap + ygap * ygap < min_spacing * min_spacing) { self_conflict = true; break; }
            }
            if (self_conflict) break;
        }
        if (self_conflict) out.push_back({items[i].var, items[i].var});
    }
    return out;
}


// Same architecture gap as find_active/gate_geometry_conflicts(), for M1:
// via_enc-M1 enclosure pads (Cont-Active-M1 or Cont-Gate-M1) are added at
// GDS-emission time with no SAT awareness. Unlike Active/Gate, M1's own
// S2S spacing (180) already matches the real DRC value and constrains the
// routed wire itself -- what it can't see is a via_enc pad relative to
// *other* via_enc pads or the wire it's supposed to sit on.
std::vector<std::pair<std::string, std::string>> find_m1_geometry_conflicts(
        const Config& cfg, const PreLayout& pre, const RoutingResult& r) {
    std::vector<std::pair<std::string, std::string>> out;
    if (!cfg.opt_bool("bulk_planar")) return out;
    const int m1_z = cfg.routing_layer_index("M1");
    if (m1_z < 0) return out;
    // Real IHP rule M1.e ("Min. space of Metal1 lines if at least one line
    // is wider than 0.15um and the parallel run is more than 1.0um": 0.22um)
    // is stricter than the plain M1.b rule (0.18um) whenever one side is a
    // wide/long run. Kept at the safe 0.18um here deliberately -- raising it
    // to 0.22um uniformly was tried and reverted (regressed nand2_4). The one
    // case guaranteed to qualify (the power rail) gets the real 0.22um value
    // via the dedicated wire-vs-rail check below (m1_rail_e_spacing); a
    // non-rail wide/long M1.e pair is a known open gap, not covered here.
    const double min_spacing = (double)cfg.opt_long("m1_wide_min_spacing");
    if (min_spacing <= 0) return out;
    const double ch = (double)cfg.cell_height;
    const double power_w = cfg.power_width("M1");

    struct GeomVar { double x0, y0, x1, y1; std::string var; long px, py; };
    std::vector<GeomVar> items;

    std::vector<std::array<double, 4>> fixed_m1;
    for (double rail : {0.0, ch})
        fixed_m1.push_back({0.0, rail - power_w / 2.0, 1e9, rail + power_w / 2.0});
    const long m1_width = cfg.width("M1");
    for (const auto& e : r.metals) {
        const auto& a = e.first; const auto& b = e.second;
        if (a[2] != m1_z || b[2] != m1_z) continue;
        const double hw = m1_width / 2.0;
        double x0 = std::min(a[0], b[0]) - hw, x1 = std::max(a[0], b[0]) + hw;
        double y0 = std::min(a[1], b[1]) - hw, y1 = std::max(a[1], b[1]) + hw;
        fixed_m1.push_back({x0, y0, x1, y1});
    }

    // The M1 side of an Active/Gate-M1 via gets a plain get_square(...,
    // "M1", ...) pad (gds.cpp's emit_metals), sized by M1's own
    // width/extension -- NOT the Active-specific bulk-enclosure formula
    // (that only ever draws on the Active layer). Replicate get_square()'s
    // point-form sizing rule here.
    {
        const int m1_dir = cfg.routing_layer_index("M1") >= 0
                                ? cfg.routing_directions[cfg.routing_layer_index("M1")] : 2;
        const long m1_ext = cfg.rules.at("extension").at("M1").get<long>();
        const long m1_w = cfg.width("M1");
        for (const auto& e : r.metals) {
            const auto& a = e.first; const auto& b = e.second;
            if (a[2] == b[2]) continue;
            const auto& lo = (a[2] < b[2]) ? a : b;
            const auto& hi = (a[2] < b[2]) ? b : a;
            if (cfg.routing_layers[hi[2]] != "M1") continue;
            if (lo[0] != hi[0] || lo[1] != hi[1]) continue;
            const std::string& lower_layer = cfg.routing_layers[lo[2]];
            if (lower_layer != "Active" && lower_layer != "Gate") continue;
            const double cx = (double)lo[0], cy = (double)lo[1];
            long x_ex, y_ex;
            if ((long)cy % (long)ch == 0 && cfg.is_power_layer("M1")) {
                x_ex = m1_w; y_ex = cfg.power_width("M1");
            } else {
                x_ex = (m1_dir == 0 /* HORIZONTAL */) ? m1_ext : m1_w;
                y_ex = (m1_dir == 1 /* VERTICAL */) ? m1_ext : m1_w;
            }
            items.push_back({cx - x_ex / 2.0, cy - y_ex / 2.0, cx + x_ex / 2.0, cy + y_ex / 2.0,
                             edge_var_name(a, b), lo[0], lo[1]});
        }
    }

    for (const auto& v : r.via_enc) {
        if (v.z != m1_z) continue;
        const auto& via = (v.level == 0) ? cfg.lower_via.at("M1") : cfg.upper_via.at("M1");
        if (!via.has_value() || via->empty()) continue;
        const long metal_width = (v.y % (long)ch == 0 && cfg.is_power_layer("M1"))
                                      ? cfg.power_width("M1") : cfg.width("M1");
        const long metal_ex = cfg.rules.at("extension").at("M1").get<long>();
        const long via_width = cfg.width(*via);
        long enc = 0;
        const auto& enclosure_rules = cfg.rules.at("enclosure");
        if (enclosure_rules.contains(*via)) {
            const auto& via_rules = enclosure_rules.at(*via);
            if (via_rules.contains("M1")) enc = via_rules.at("M1").get<long>();
        }
        // Mirrors gds.cpp: the cross axis covers the cut, not the bare width.
        // Cross axis clears V1.c and drops below V1.c1's enclosed<0.01 guard.
        const long cross_enc = cfg.opt_long("via_cross_enclosure", 20);
        const long cross = (via_width > metal_width) ? via_width + 2 * cross_enc : metal_width;
        // M1.d: the pad also has to meet the metal minimum area, and a 290 by
        // 230 pad is 0.0667 against 0.09. Grow the along axis to whatever the
        // area needs, on the manufacturing grid.
// Mirrors gds.cpp: the minimum-area repair is the gate-contact square, not this pad.
        const long metal_ex_a = metal_ex;
        long xe, ye, xmw, ymw;
        if (v.dir == 0) { xe = enc; ye = 0; xmw = metal_ex_a; ymw = cross; }
        else { xe = 0; ye = enc; xmw = cross; ymw = metal_ex_a; }
        const double ah = std::max(via_width / 2.0 + (double)std::max(xe, ye),
                                   metal_ex_a / 2.0);
        const double lx = (xe != 0) ? v.x - ah : v.x - xmw / 2.0;
        const double ux = (xe != 0) ? v.x + ah : v.x + xmw / 2.0;
        const double ly = (ye != 0) ? v.y - ah : v.y - ymw / 2.0;
        const double uy = (ye != 0) ? v.y + ah : v.y + ymw / 2.0;
        std::string dir_s = (v.dir == 0) ? "HOR" : "VER";
        std::string lvl_s = (v.level == 0) ? "L" : "U";
        std::string var = "VIA_ENC_" + dir_s + "_" + lvl_s + "_G_x" + std::to_string(v.x) +
                          "y" + std::to_string(v.y) + "z" + std::to_string(v.z);
        items.push_back({lx, ly, ux, uy, var, v.x, v.y});
    }
    if (items.empty()) return out;

    using D4 = std::array<double, 4>;
    auto gap_1d = [](double a0, double a1, double b0, double b1) {
        if (a1 <= b0) return b0 - a1;
        if (b1 <= a0) return a0 - b1;
        return -1.0;
    };
    auto protrusions_of = [&](size_t self_idx) {
        const GeomVar& it = items[self_idx];
        std::vector<D4> pr;
        bool any = false;
        double rx0 = it.x0, ry0 = it.y0, rx1 = it.x1, ry1 = it.y1;
        for (const auto& reg : fixed_m1) {
            if (reg[0] >= it.x1 || reg[2] <= it.x0 || reg[1] >= it.y1 || reg[3] <= it.y0) continue;
            if (!any) { rx0 = reg[0]; ry0 = reg[1]; rx1 = reg[2]; ry1 = reg[3]; any = true; }
            else {
                rx0 = std::min(rx0, reg[0]); ry0 = std::min(ry0, reg[1]);
                rx1 = std::max(rx1, reg[2]); ry1 = std::max(ry1, reg[3]);
            }
        }
        for (size_t k = 0; k < items.size(); ++k) {
            if (k == self_idx) continue;
            if (items[k].px == it.px && items[k].py == it.py) continue;
            const double reg0 = items[k].x0, reg1 = items[k].y0, reg2 = items[k].x1, reg3 = items[k].y1;
            if (reg0 >= it.x1 || reg2 <= it.x0 || reg1 >= it.y1 || reg3 <= it.y0) continue;
            if (!any) { rx0 = reg0; ry0 = reg1; rx1 = reg2; ry1 = reg3; any = true; }
            else {
                rx0 = std::min(rx0, reg0); ry0 = std::min(ry0, reg1);
                rx1 = std::max(rx1, reg2); ry1 = std::max(ry1, reg3);
            }
        }
        if (!any) { pr.push_back({it.x0, it.y0, it.x1, it.y1}); return pr; }
        if (rx0 - it.x0 > 0) pr.push_back({it.x0, it.y0, rx0, it.y1});
        if (it.x1 - rx1 > 0) pr.push_back({rx1, it.y0, it.x1, it.y1});
        if (ry0 - it.y0 > 0) pr.push_back({it.x0, it.y0, it.x1, ry0});
        if (it.y1 - ry1 > 0) pr.push_back({it.x0, ry1, it.x1, it.y1});
        auto covered = [&](const D4& p) {
            for (const auto& reg : fixed_m1)
                if (p[0] >= reg[0] && p[2] <= reg[2] && p[1] >= reg[1] && p[3] <= reg[3]) return true;
            for (size_t k = 0; k < items.size(); ++k) {
                if (k == self_idx) continue;
                if (items[k].px == it.px && items[k].py == it.py) continue;
                if (p[0] >= items[k].x0 && p[2] <= items[k].x1 && p[1] >= items[k].y0 && p[3] <= items[k].y1) return true;
            }
            return false;
        };
        pr.erase(std::remove_if(pr.begin(), pr.end(), covered), pr.end());
        return pr;
    };

    std::vector<std::vector<D4>> all_pr(items.size());
    for (size_t i = 0; i < items.size(); ++i) all_pr[i] = protrusions_of(i);

    std::set<std::pair<std::string, std::string>> seen_pairs;
    for (size_t i = 0; i < items.size(); ++i) {
        if (all_pr[i].empty()) continue;
        for (size_t j = i + 1; j < items.size(); ++j) {
            if (all_pr[j].empty()) continue;
            if (items[i].var == items[j].var) continue;
            if (items[i].px == items[j].px && items[i].py == items[j].py) continue;
            bool conflict = false;
            for (const auto& p1 : all_pr[i]) {
                for (const auto& p2 : all_pr[j]) {
                    const double xgap = gap_1d(p1[0], p1[2], p2[0], p2[2]);
                    const double ygap = gap_1d(p1[1], p1[3], p2[1], p2[3]);
                    // gap_1d returns 0 for shapes that touch on an axis. Touching merges
                    // them into one polygon, so a separation on the other axis is an
                    // interior notch the deck reports as M1.b. Measured on sg13g2_xnor2_4:
                    // a HOR pad at (3000,620) and a VER pad at (2740,1030) meet at x=2855
                    // with a 150nm y gap, and strict < skipped exactly that. A corner
                    // touch (both gaps 0) still passes, since neither branch takes it.
                    if (xgap <= 0 && ygap > 0 && ygap < min_spacing) { conflict = true; break; }
                    if (ygap <= 0 && xgap > 0 && xgap < min_spacing) { conflict = true; break; }
                    if (xgap > 0 && ygap > 0 && xgap * xgap + ygap * ygap < min_spacing * min_spacing) { conflict = true; break; }
                }
                if (conflict) break;
            }
            if (!conflict) continue;
            auto key = (items[i].var < items[j].var) ? std::make_pair(items[i].var, items[j].var)
                                                      : std::make_pair(items[j].var, items[i].var);
            seen_pairs.insert(key);
            out.push_back(key);
        }
    }
    for (size_t i = 0; i < items.size(); ++i) {
        if (all_pr[i].empty()) continue;
        bool self_conflict = false;
        for (const auto& p1 : all_pr[i]) {
            for (const auto& reg : fixed_m1) {
                const double xgap = gap_1d(p1[0], p1[2], reg[0], reg[2]);
                const double ygap = gap_1d(p1[1], p1[3], reg[1], reg[3]);
                if (xgap < 0 && ygap > 0 && ygap < min_spacing) { self_conflict = true; break; }
                if (ygap < 0 && xgap > 0 && xgap < min_spacing) { self_conflict = true; break; }
                if (xgap > 0 && ygap > 0 && xgap * xgap + ygap * ygap < min_spacing * min_spacing) { self_conflict = true; break; }
            }
            if (self_conflict) break;
        }
        if (self_conflict) {
            out.push_back({items[i].var, items[i].var});
        }
    }

    {
        std::vector<std::pair<std::array<long, 2>, std::array<long, 2>>> m1_edges_2d;
        std::vector<std::pair<std::array<long, 3>, std::array<long, 3>>> m1_edges_3d;
        for (const auto& e : r.metals) {
            const auto& a = e.first; const auto& b = e.second;
            if (a[2] != m1_z || b[2] != m1_z) continue;
            m1_edges_2d.push_back({{a[0], a[1]}, {b[0], b[1]}});
            m1_edges_3d.push_back(e);
        }
        std::vector<std::pair<Rect, std::string>> wires;
        for (const auto& span : merge_collinear(m1_edges_2d)) {
            std::string rep_var;
            const bool vertical = span.first[0] == span.second[0];
            for (const auto& e : m1_edges_3d) {
                const auto& a = e.first; const auto& b = e.second;
                if (vertical) {
                    if (a[0] != span.first[0] || b[0] != span.first[0]) continue;
                    if (std::min(a[1], b[1]) < span.first[1] || std::max(a[1], b[1]) > span.second[1]) continue;
                } else {
                    if (a[1] != span.first[1] || b[1] != span.first[1]) continue;
                    if (std::min(a[0], b[0]) < span.first[0] || std::max(a[0], b[0]) > span.second[0]) continue;
                }
                rep_var = edge_var_name(a, b);
                break;
            }
            if (rep_var.empty()) continue;
            Rect sq = get_square(cfg, span.first[0], span.first[1], span.second[0], span.second[1], "M1", pre);
            wires.push_back({sq, rep_var});
        }
        auto gap_1d_w = [](double a0, double a1, double b0, double b1) {
            if (a1 <= b0) return b0 - a1;
            if (b1 <= a0) return a0 - b1;
            return -1.0;
        };
        auto too_close = [&](const Rect& p1, const Rect& p2, double spacing) {
            const double xgap = gap_1d_w(p1[0], p1[2], p2[0], p2[2]);
            const double ygap = gap_1d_w(p1[1], p1[3], p2[1], p2[3]);
            if (xgap < 0 && ygap > 0 && ygap < spacing) return true;
            if (ygap < 0 && xgap > 0 && xgap < spacing) return true;
            if (xgap > 0 && ygap > 0 && xgap * xgap + ygap * ygap < spacing * spacing) return true;
            return false;
        };
        const double rail_spacing = (double)cfg.opt_long("m1_rail_e_spacing");
        const double rail_e_min_run = (double)cfg.opt_long("m1_rail_e_min_run");
        for (double rail : {0.0, ch}) {
            Rect rail_rect{0.0, rail - power_w / 2.0, 1e9, rail + power_w / 2.0};
            for (const auto& w : wires) {
                const double run = w.first[2] - w.first[0];
                const double spacing = (rail_spacing > 0 && run >= rail_e_min_run)
                                           ? rail_spacing : min_spacing;
                if (too_close(w.first, rail_rect, spacing)) out.push_back({w.second, w.second});
            }
        }
        for (size_t i = 0; i < wires.size(); ++i)
            for (size_t j = i + 1; j < wires.size(); ++j)
                if (too_close(wires[i].first, wires[j].first, min_spacing))
                    out.push_back(wires[i].second < wires[j].second
                                      ? std::make_pair(wires[i].second, wires[j].second)
                                      : std::make_pair(wires[j].second, wires[i].second));
    }
    return out;
}


// Under bulk planar a Gate-M1 Cont is never drawn (gds.cpp's
// skip_bulk_gate_to_m1), so M1 cannot join two gate fingers in the emitted
// layout even though the routing graph offers the via edges. A net routed
// that way extracts as several disconnected gates (found via
// sg13g2_o21ai_2: five fingers held together only by vias on one track).
// One via per connected gate group is fine -- that is the external pin
// stub. Flag every pair of vias whose gate groups have no Gate-layer path
// between them, so CEGAR forbids using M1 as an inter-finger bridge.
std::vector<std::pair<std::string, std::string>> find_gate_bridge_conflicts(
    const Config& cfg, const RoutingResult& r) {
    std::vector<std::pair<std::string, std::string>> out;
    if (!cfg.opt_bool("bulk_planar")) return out;
    const int gate_z = cfg.routing_layer_index(cfg.gate_contact_layer);
    const int m1_z = cfg.routing_layer_index("M1");
    if (gate_z < 0 || m1_z < 0) return out;
    using Node = std::array<long, 3>;
    std::map<Node, std::vector<Node>> adj;
    for (const auto& e : r.metals) {
        adj[e.first].push_back(e.second);
        adj[e.second].push_back(e.first);
    }
    std::set<Node> seen;
    std::set<std::pair<std::string, std::string>> pairs;
    for (const auto& kv : adj) {
        if (seen.count(kv.first)) continue;
        std::vector<Node> comp{kv.first};
        seen.insert(kv.first);
        for (size_t qi = 0; qi < comp.size(); ++qi)
            for (const Node& nxt : adj[comp[qi]])
                if (seen.insert(nxt).second) comp.push_back(nxt);
        std::set<Node> in_comp(comp.begin(), comp.end());
        // Gate columns are drawn full height, so any two routed gate nodes
        // on the same x are already joined; only inter-column routed spans
        // merge different columns.
        std::map<long, long> parent;
        std::function<long(long)> find_root = [&](long x) {
            while (parent[x] != x) x = parent[x] = parent[parent[x]];
            return x;
        };
        auto ensure = [&](long x) { if (!parent.count(x)) parent[x] = x; };
        for (const Node& n : comp)
            if (n[2] == gate_z) ensure(n[0]);
        for (const auto& e : r.metals) {
            if (!in_comp.count(e.first)) continue;
            if (e.first[2] == gate_z && e.second[2] == gate_z && e.first[0] != e.second[0]) {
                ensure(e.first[0]); ensure(e.second[0]);
                parent[find_root(e.first[0])] = find_root(e.second[0]);
            }
        }
        std::map<long, std::vector<std::string>> vias_by_group;
        for (const auto& e : r.metals) {
            if (!in_comp.count(e.first)) continue;
            const long za = e.first[2], zb = e.second[2];
            const bool is_gate_m1 = (za == gate_z && zb == m1_z) || (za == m1_z && zb == gate_z);
            if (!is_gate_m1) continue;
            const long gx = (za == gate_z) ? e.first[0] : e.second[0];
            ensure(gx);
            vias_by_group[find_root(gx)].push_back(edge_var_name(e.first, e.second));
        }
        if (vias_by_group.size() < 2) continue;
        std::vector<const std::vector<std::string>*> groups;
        for (const auto& g : vias_by_group) groups.push_back(&g.second);
        for (size_t i = 0; i < groups.size(); ++i)
            for (size_t j = i + 1; j < groups.size(); ++j)
                for (const auto& a : *groups[i])
                    for (const auto& b : *groups[j])
                        pairs.insert(a < b ? std::make_pair(a, b) : std::make_pair(b, a));
    }
    out.assign(pairs.begin(), pairs.end());
    return out;
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
    bool any_undecided = false;
    bool any_geometry_unresolved = false;
    for (const std::string& top : candidates) {
        Config cfg_t = cfg;
        auto it = std::find(cfg_t.routing_layers.begin(), cfg_t.routing_layers.end(), top);
        if (it != cfg_t.routing_layers.end()) cfg_t.routing_layers.erase(it + 1, cfg_t.routing_layers.end());

        auto xp = get_x_points(cfg_t, (long)no.n_net.size());
        auto yp = get_y_points(cfg_t, cfg_t.opt_bool("allow_below_min_track"), &no);
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
        {
            // A power net's source/drain contact can be tied straight to the
            // rail through the Active layer itself (see gds.cpp
            // power_metals()). That tie extends Active beyond the plain
            // per-column channel range add_active() computes from fin
            // counts alone, and the routing graph has no other way to learn
            // about it before routing runs. Without this, a Gate-layer
            // signal wire routed through the resulting gap can be drawn
            // directly across the tie's Active, forming a parasitic
            // transistor that only DRC/LVS catch after the fact (found via
            // sg13g2_nor3_4 / sg13g2_nand3_2).
            const int active_z = cfg_t.routing_layer_index("Active");
            const long active_w = cfg_t.width("Active");
            for (const Net& net : cnets) {
                if (!net.is_power || net.pins.size() < 2) continue;
                const Pin& contact = net.pins[0];
                const Pin& rail_pin = net.pins[1];
                if (rail_pin.points.empty() || contact.points.empty()) continue;
                const long rail_y = rail_pin.points[0].y;
                const Point* closest = &contact.points[0];
                for (const Point& p : contact.points)
                    if (std::labs(p.y - rail_y) < std::labs(closest->y - rail_y)) closest = &p;
                if (closest->z != active_z) continue;
                const double lo = std::min((double)closest->y, (double)rail_y) - active_w / 2.0;
                const double hi = std::max((double)closest->y, (double)rail_y) + active_w / 2.0;
                pre.active.push_back({(double)closest->x - active_w / 2.0, lo,
                                      (double)closest->x + active_w / 2.0, hi});
            }
        }
        auto rg = build_routing_graph(cfg_t, xp, yp.y_points, yp.ext_pin_y_tracks, pre);
        {
            // Active has no in-layer edges, so an Active-M1 via helps a net
            // only at one of its own Active pin points -- anywhere else the
            // Active side is a dead end, yet nothing penalizes leaving the
            // via true, and emission then draws a Cont plus a bulk Active
            // square there. On a gate column that Cont lands on the
            // full-height poly and merges the gate net with the diffusion
            // nets (found via sg13g2_o21ai_2: VSS|net1|A2). Keep Active-M1
            // via candidates only at real Active pin points.
            const int active_z = cfg_t.routing_layer_index(cfg_t.active_contact_layer);
            if (active_z >= 0) {
                std::set<std::array<long, 3>> active_pins;
                for (const auto& n : cnets)
                    for (const auto& p : n.pins)
                        for (const auto& pt : p.points)
                            if (pt.z == active_z) active_pins.insert({pt.x, pt.y, pt.z});
                std::set<std::pair<std::array<long, 3>, std::array<long, 3>>> kept;
                for (const auto& e : rg.edges) {
                    const auto& a = e.first;
                    const auto& b = e.second;
                    const bool active_via = a[2] != b[2] && std::min(a[2], b[2]) == active_z;
                    if (active_via && !active_pins.count({a[0], a[1], (long)active_z})) continue;
                    kept.insert(e);
                }
                rg.edges.swap(kept);
            }
        }
        std::vector<std::pair<std::string, std::string>> forbidden;
        RoutingResult r;
        RoutingResult best_r;
        size_t best_total = std::numeric_limits<size_t>::max();
        bool have_best = false;
        for (int attempt = 0; attempt < 50; ++attempt) {
            r = solve_cell(cfg_t, xp, yp.y_points, yp.ext_pin_y_tracks, rg, cnets, pre, no,
                           debug_dump_default, forbidden);
            if (!r.sat) break;
            auto conflicts = find_active_geometry_conflicts(cfg_t, pre, r);
            auto gate_conflicts = find_gate_geometry_conflicts(cfg_t, no, pre, r);
            auto m1_conflicts = find_m1_geometry_conflicts(cfg_t, pre, r);
            const size_t total = conflicts.size() + gate_conflicts.size() + m1_conflicts.size();
            // Blocking one category's conflict can perturb the solution
            // into a worse spot for the other category (non-monotonic) --
            // keep whichever attempt had the fewest total conflicts seen so
            // far, not just whatever the last attempt happened to produce.
            const size_t total_before = best_total;
            if (!have_best || total <= best_total) { best_r = r; best_total = total; have_best = true; }
            if (total == 0) break;
            if (total <= total_before) {
                // This round is at least as good as anything seen before --
                // trust its whole conflict set as a basis for exclusion.
                for (auto& c : conflicts) forbidden.push_back(c);
                for (auto& c : gate_conflicts) forbidden.push_back(c);
                for (auto& c : m1_conflicts) forbidden.push_back(c);
            } else {
                // Worse than what we already have -- a detour, not a
                // trustworthy basis for exclusion. Still forbid one pair so
                // the next attempt can't just reproduce this identical
                // solution, but don't dump the whole set and risk pushing an
                // already-tight cell into genuine UNSAT before the attempt
                // budget is used up.
                constexpr size_t kWorseningCap = 6;
                size_t added = 0;
                for (auto& c : conflicts) { if (added >= kWorseningCap) break; forbidden.push_back(c); ++added; }
                for (auto& c : gate_conflicts) { if (added >= kWorseningCap) break; forbidden.push_back(c); ++added; }
                for (auto& c : m1_conflicts) { if (added >= kWorseningCap) break; forbidden.push_back(c); ++added; }
            }
        }
        if (r.undecided) any_undecided = true;
        if (have_best) r = best_r;
        if (have_best && best_total != 0) {
            std::cerr << "[GEOMETRY_UNRESOLVED] top_layer=" << top
                      << " conflicts=" << best_total << std::endl;
            r.sat = false;
            // A bounded refinement search is not a proof of infeasibility.
            any_undecided = true;
            any_geometry_unresolved = true;
        }
        r.top_layer = top;
        r.undecided = any_undecided;
        r.geometry_unresolved = !r.sat && any_geometry_unresolved;
        if (r.sat) return r;
        last = r;
        last.top_layer = top;
    }
    last.undecided = any_undecided;
    return last;
}

}
