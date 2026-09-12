// Per-net constraints: layer exclusivity, edge assignment, commodity flow,
// minimum pin length, and pre-layout blockage avoidance.
#include "smt.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include "logic.hpp"
#include "variables.hpp"
#include "placement.hpp"

namespace aumedal {
namespace {
using Arr = std::array<long, 3>;
constexpr double DMAX = 1e18;

struct NetInfo {
    const Net* net;
    std::string cname;
    bool is_power, is_dummy, is_ext_pin;
    double min_x, max_x, min_y, max_y;
    int commodity_count;
    std::set<std::array<long, 3>> pinset;
};

std::string collapse_name(const Net& n, const std::string& pwr, const std::string& gnd) {
    if (n.is_power) {
        if (n.name.find(pwr) != std::string::npos) return pwr;
        if (n.name.find(gnd) != std::string::npos) return gnd;
    }
    return n.name;
}

std::vector<NetInfo> build_net_infos(const Config& cfg, const std::vector<Net>& nets, long tol = 0,
                                     double x_unit = 0.0, double y_unit = 0.0) {
    std::vector<NetInfo> out;
    out.reserve(nets.size());
    for (const Net& n : nets) {
        NetInfo ni;
        ni.net = &n;
        ni.cname = collapse_name(n, cfg.power_net, cfg.ground_net);
        ni.is_power = n.is_power;
        ni.is_dummy = (n.name == DUMMY_NET);
        ni.is_ext_pin = n.is_ext_pin;
        ni.commodity_count = (int)n.pins.size() - 1;
        long bx_min = std::numeric_limits<long>::max(), by_min = std::numeric_limits<long>::max();
        long bx_max = 0, by_max = 0;
        for (const Pin& p : n.pins)
            for (const Point& pt : p.points) {
                bx_min = std::min(bx_min, pt.x); bx_max = std::max(bx_max, pt.x);
                by_min = std::min(by_min, pt.y); by_max = std::max(by_max, pt.y);
            }
        if (n.is_power) {
            ni.min_x = 0; ni.max_x = DMAX; ni.min_y = by_min; ni.max_y = by_max;
        } else {
            const long ex = (n.name == DUMMY_NET) ? 0 : tol;
            ni.min_x = bx_min - ex * x_unit; ni.max_x = bx_max + ex * x_unit;
            ni.min_y = by_min - ex * y_unit; ni.max_y = by_max + ex * y_unit;
        }
        if (ni.is_dummy)
            for (const Pin& p : n.pins)
                for (const Point& pt : p.points) ni.pinset.insert({pt.x, pt.y, pt.z});
        out.push_back(std::move(ni));
    }
    return out;
}

bool in_bbox(const NetInfo& ni, const Arr& p) {
    return (double)p[0] >= ni.min_x && (double)p[0] <= ni.max_x &&
           (double)p[1] >= ni.min_y && (double)p[1] <= ni.max_y;
}
bool net_has_point(const NetInfo& ni, const RoutingGraph& g, const Arr& p) {
    return g.nodes.count(p) > 0 && in_bbox(ni, p);
}
bool name_has(const std::string& s, const char* sub) { return s.find(sub) != std::string::npos; }

std::map<Arr, std::vector<Arr>> net_adjacency(const NetInfo& ni, const RoutingGraph& g) {
    std::map<Arr, std::vector<Arr>> adj;
    for (const auto& e : g.edges) {
        if (in_bbox(ni, e.first) && in_bbox(ni, e.second)) {
            adj[e.first].push_back(e.second);
            adj[e.second].push_back(e.first);
        }
    }
    return adj;
}
std::vector<Arr> net_point_keys(const NetInfo& ni, const RoutingGraph& g) {
    std::vector<Arr> v;
    for (const auto& p : g.nodes) {
        if (!in_bbox(ni, p)) continue;
        if (ni.is_dummy && !ni.pinset.count(p)) continue;
        v.push_back(p);
    }
    return v;
}
z3::expr at_most(z3::context& ctx, const std::vector<z3::expr>& es, unsigned k) {
    z3::expr_vector v(ctx);
    for (const auto& e : es) v.push_back(e);
    return z3::atmost(v, k);
}
z3::expr at_least(z3::context& ctx, const std::vector<z3::expr>& es, unsigned k) {
    z3::expr_vector v(ctx);
    for (const auto& e : es) v.push_back(e);
    return z3::atleast(v, k);
}
z3::expr exactly(z3::context& ctx, const std::vector<z3::expr>& es, unsigned n) {
    return at_most(ctx, es, n) && at_least(ctx, es, n);
}
z3::expr not_exactly_one(z3::context& ctx, const std::vector<z3::expr>& vars) {
    using logic::Cond;
    std::vector<Cond> ands;
    ands.push_back(Cond::Expr(at_most(ctx, vars, 2)));
    std::vector<Cond> or_conds;
    for (size_t i = 0; i < vars.size(); ++i) {
        std::vector<Cond> terms;
        for (size_t j = 0; j < vars.size(); ++j)
            terms.push_back(j == i ? logic::Not(Cond::Expr(vars[j])) : Cond::Expr(vars[j]));
        or_conds.push_back(logic::Or(ctx, terms));
    }
    ands.push_back(logic::And(ctx, or_conds));
    Cond r = logic::And(ctx, ands);
    return *r.e;
}
}

double spacing0(const Config& c, const char* kind, const std::string& a, const std::string& b) {
    const auto& sp = c.rules.at("spacing");
    if (!sp.contains(kind)) return 0;
    const auto& k = sp.at(kind);
    if (!k.contains(a)) return 0;
    const auto& ka = k.at(a);
    if (!ka.contains(b)) return 0;
    return ka.at(b).get<double>();
}
std::set<Arr> compute_via_points_net(const Config& cfg, const std::vector<std::vector<long>>& x_points,
                                     const std::vector<std::vector<long>>& y_points, const RoutingGraph& g) {
    std::map<std::string, int> layer_index;
    for (int i = 0; i < (int)cfg.routing_layers.size(); ++i) layer_index[cfg.routing_layers[i]] = i;
    std::map<std::string, int> via_index;
    for (int i = 0; i < (int)cfg.vias.size(); ++i) via_index[cfg.vias[i]] = i;
    std::map<Arr, int> cnt;
    std::set<Arr> res;
    for (int z = 0; z < (int)cfg.routing_layers.size(); ++z) {
        const std::string& layer = cfg.routing_layers[z];
        std::vector<int> upper_zs;
        for (auto& ul : cfg.upper_layers.at(layer)) if (layer_index.count(ul)) upper_zs.push_back(layer_index[ul]);
        int max_same = -1;
        for (auto& sh : cfg.same_height_layers.at(layer)) if (layer_index.count(sh)) max_same = std::max(max_same, layer_index[sh]);
        auto uv = cfg.upper_via.find(layer);
        if (uv == cfg.upper_via.end() || !uv->second.has_value()) continue;
        auto vit = via_index.find(*uv->second);
        if (vit == via_index.end()) continue;
        const int vidx = vit->second;
        for (long x : x_points[z]) for (long y : y_points[z]) {
            const Arr p{x, y, z}, vk{x, y, vidx};
            for (int uz : upper_zs) if (edge_in_graph(g, p, Arr{x, y, uz})) cnt[vk]++;
            if (z == max_same) res.insert(vk);
        }
    }
    return res;
}

void add_ext_pin_points(std::vector<Net>& nets, const Config& cfg,
                        const std::vector<std::vector<long>>& x_points,
                        const std::vector<std::vector<long>>& ext_pin_y_tracks,
                        long tolerance) {
    for (Net& n : nets) {
        if (!n.is_ext_pin || n.is_power) continue;
        long min_x = std::numeric_limits<long>::max(), max_x = 0;
        long min_y = std::numeric_limits<long>::max(), max_y = 0;
        bool any = false;
        for (const Pin& p : n.pins)
            for (const Point& pt : p.points) {
                any = true;
                min_x = std::min(min_x, pt.x); max_x = std::max(max_x, pt.x);
                min_y = std::min(min_y, pt.y); max_y = std::max(max_y, pt.y);
            }
        if (!any) continue;
        min_x = std::max(0L, min_x - (long)(tolerance * cfg.x_unit));
        max_x = max_x + (long)(tolerance * cfg.x_unit);
        long max_row = (max_y - 1) / cfg.cell_height;
        long min_row = min_y / cfg.cell_height;
        std::vector<long> computed_ys;
        for (long row = min_row; row <= max_row; ++row) {
            bool is_flip = (row % 2 == 1);
            long row_min_y = row * cfg.cell_height, row_max_y = (row + 1) * cfg.cell_height;
            double contact_y = row_min_y + (row_max_y - row_min_y) / 2.0 + (is_flip ? -cfg.np_offset : cfg.np_offset);
            computed_ys.push_back((long)contact_y);
        }
        Pin ext;
        ext.term = TERM_EXT_PIN;
        for (const std::string& layer : cfg.ext_pin_layer) {
            int ext_idx = cfg.routing_layer_index(layer);
            if (ext_idx < 0 || ext_idx >= (int)x_points.size()) continue;
            for (long x : x_points[ext_idx])
                if (min_x <= x && x <= max_x)
                    for (long y : computed_ys) ext.points.push_back({x, y, ext_idx});
        }
        if (!ext.points.empty()) n.pins.push_back(std::move(ext));
    }
}

void add_access_points(std::vector<Net>& nets, const Config& cfg,
                       const std::vector<std::vector<long>>& x_points,
                       const std::vector<std::vector<long>>& y_points,
                       long tolerance) {
    if (!cfg.option.value("ensure_access_points", false)) return;
    for (Net& n : nets) {
        if (!n.is_ext_pin || n.is_power) continue;
        long min_x = std::numeric_limits<long>::max(), max_x = 0;
        long min_y = std::numeric_limits<long>::max(), max_y = 0;
        bool any = false;
        for (const Pin& p : n.pins) {
            if (p.term == TERM_EXT_PIN) continue;
            for (const Point& pt : p.points) {
                any = true;
                min_x = std::min(min_x, pt.x); max_x = std::max(max_x, pt.x);
                min_y = std::min(min_y, pt.y); max_y = std::max(max_y, pt.y);
            }
        }
        if (!any) continue;
        min_x = std::max(0L, min_x - (long)(tolerance * cfg.x_unit));
        max_x = max_x + (long)(tolerance * cfg.x_unit);
        min_y = std::max(0L, min_y - (long)(tolerance * cfg.y_unit));
        max_y = max_y + (long)(tolerance * cfg.y_unit);
        Pin acc;
        acc.term = TERM_ACCESS_POINT;
        for (const std::string& epl : cfg.ext_pin_layer) {
            auto it = cfg.upper_layers.find(epl);
            if (it == cfg.upper_layers.end()) continue;
            for (const std::string& upper : it->second) {
                int uidx = cfg.routing_layer_index(upper);
                if (uidx < 0 || uidx >= (int)x_points.size() || uidx >= (int)y_points.size()) continue;
                for (long x : x_points[uidx])
                    if (min_x <= x && x <= max_x)
                        for (long y : y_points[uidx])
                            if (min_y <= y && y <= max_y)
                                acc.points.push_back({x, y, uidx});
            }
        }
        if (!acc.points.empty()) n.pins.push_back(std::move(acc));
    }
}

std::vector<z3::expr> SmtModel::add_layer_exclusivity(
    const Config& cfg, const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points, const RoutingGraph& g, const std::vector<Net>& nets) {
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    auto with_edges = points_with_edges(g);
    std::map<std::string, int> layer_index;
    for (int i = 0; i < (int)cfg.routing_layers.size(); ++i) layer_index[cfg.routing_layers[i]] = i;
    std::set<std::string> no_overlap;
    if (cfg.rules.contains("no_overlap"))
        for (const auto& l : cfg.rules.at("no_overlap")) no_overlap.insert(l.get<std::string>());

    auto base_edge = [&](const Arr& a, const Arr& b) -> std::optional<z3::expr> {
        if (!edge_in_graph(g, a, b)) return std::nullopt;
        return bool_var(edge_var_name(a, b));
    };
    auto getpos = [](const std::vector<long>& v, long i) -> std::optional<long> {
        return (i >= 0 && i < (long)v.size()) ? std::optional<long>(v[i]) : std::nullopt;
    };

    std::vector<z3::expr> out;
    const int nz = (int)cfg.routing_layers.size();
    for (int curr_z = 0; curr_z < nz; ++curr_z) {
        const std::string& curr_layer = cfg.routing_layers[curr_z];
        if (!cfg.same_height_layers.count(curr_layer)) continue;
        for (const std::string& next_layer : cfg.same_height_layers.at(curr_layer)) {
            if (!layer_index.count(next_layer)) continue;
            int next_z = layer_index[next_layer];
            if (curr_z >= next_z) continue;
            const auto& xs = x_points[curr_z];
            const auto& ys = y_points[curr_z];
            for (size_t xi = 0; xi < xs.size(); ++xi) {
                const long x = xs[xi];
                for (size_t yi = 0; yi < ys.size(); ++yi) {
                    const long y = ys[yi];
                    const auto rx = getpos(xs, (long)xi + 1), ty = getpos(ys, (long)yi + 1);
                    const Arr u{x, y, curr_z}, v{x, y, next_z};
                    if (!with_edges.count(u) || !with_edges.count(v)) continue;

                    for (const NetInfo& n1 : infos) {
                        if (n1.is_dummy || n1.is_power) continue;
                        if (!net_has_point(n1, g, u)) continue;
                        z3::expr u_var = bool_var(net_point_var_name(n1.cname, x, y, curr_z));
                        for (const NetInfo& n2 : infos) {
                            if (n2.is_dummy || n2.is_power) continue;
                            if (n1.net == n2.net
                                || (name_has(n1.net->name, cfg.power_net.c_str()) && name_has(n2.net->name, cfg.power_net.c_str()))
                                || (name_has(n1.net->name, cfg.ground_net.c_str()) && name_has(n2.net->name, cfg.ground_net.c_str()))) {
                                if (!no_overlap.count(curr_layer) && !no_overlap.count(next_layer)) continue;
                            }
                            if (!net_has_point(n2, g, v)) continue;
                            z3::expr v_var = bool_var(net_point_var_name(n2.cname, x, y, next_z));
                            out.push_back((!(u_var && v_var)));
                        }
                    }
                    if (rx) {
                        auto re1 = base_edge(u, {*rx, y, curr_z}), re2 = base_edge(v, {*rx, y, next_z});
                        if (re1 && re2) out.push_back((!(*re1 && *re2)));
                    }
                    if (ty) {
                        auto te1 = base_edge(u, {x, *ty, curr_z}), te2 = base_edge(v, {x, *ty, next_z});
                        if (te1 && te2) out.push_back((!(*te1 && *te2)));
                    }
                }
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_edge_assignment(const Config& cfg, const RoutingGraph& g,
                                                       const std::vector<Net>& nets) {
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    std::vector<z3::expr> out;
    for (const NetInfo& ni : infos) {
        if (ni.is_dummy || ni.is_power) continue;
        if (ni.commodity_count <= 0) continue;
        for (const auto& e : g.edges) {
            if (!in_bbox(ni, e.first) || !in_bbox(ni, e.second)) continue;
            const Arr& a = e.first;
            const Arr& b = e.second;
            z3::expr edge_var = bool_var(net_edge_var_name(ni.cname, a, b));
            for (int c = 0; c < ni.commodity_count; ++c) {
                z3::expr comm = bool_var(comm_edge_var_name(ni.cname, c, a, b));
                out.push_back(((!comm) || edge_var));
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_vertex_exclusivity(const Config& cfg, const RoutingGraph& g,
                                                          const std::vector<Net>& nets) {
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    std::vector<z3::expr> out;
    for (const NetInfo& ni : infos) {
        auto adj = net_adjacency(ni, g);
        for (const Arr& p : net_point_keys(ni, g)) {
            z3::expr pv = bool_var(net_point_var_name(ni.cname, p[0], p[1], p[2]));
            std::vector<logic::Cond> edges;
            auto it = adj.find(p);
            if (it != adj.end())
                for (const Arr& q : it->second) edges.push_back(logic::Cond::Expr(bool_var(net_edge_var_name(ni.cname, p, q))));
            if (!edges.empty())
                out.push_back(logic::eq_expr(ctx, pv, logic::Or(ctx, edges)));
            else
                out.push_back((pv == ctx.bool_val(false)));
        }
    }
    for (const Arr& p : g.nodes) {
        std::vector<z3::expr> vars;
        std::set<unsigned> seen_ids;
        for (const NetInfo& ni : infos)
            if (in_bbox(ni, p) && (!ni.is_dummy || ni.pinset.count(p))) {
                z3::expr v = bool_var(net_point_var_name(ni.cname, p[0], p[1], p[2]));
                if (seen_ids.insert(v.id()).second) vars.push_back(v);
            }
        if (!vars.empty()) out.push_back(at_most(ctx, vars, 1));
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_metal_segment(const Config& cfg, const RoutingGraph& g,
                                                     const std::vector<Net>& nets) {
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    std::map<Arr, std::vector<Arr>> badj;
    for (const auto& e : g.edges) { badj[e.first].push_back(e.second); badj[e.second].push_back(e.first); }
    std::vector<z3::expr> out;
    for (const Arr& point : g.nodes) {
        z3::expr metal_point_var = bool_var(point_var_name("G", point[0], point[1], point[2]));
        std::vector<logic::Cond> incident_metal_edges;
        auto it = badj.find(point);
        if (it != badj.end())
            for (const Arr& adj_point : it->second) {
                z3::expr metal_edge_var = bool_var(edge_var_name(point, adj_point));
                incident_metal_edges.push_back(logic::Cond::Expr(metal_edge_var));
                std::vector<z3::expr> net_edge_vars;
                for (const NetInfo& ni : infos) {
                    if (ni.is_dummy || ni.is_power) continue;
                    if (!in_bbox(ni, point) || !in_bbox(ni, adj_point)) continue;
                    net_edge_vars.push_back(bool_var(net_edge_var_name(ni.cname, point, adj_point)));
                }
                if (!net_edge_vars.empty()) {
                    out.push_back(at_most(ctx, net_edge_vars, 1));
                    std::vector<logic::Cond> nev;
                    for (auto& e : net_edge_vars) nev.push_back(logic::Cond::Expr(e));
                    out.push_back(logic::eq_expr(ctx, metal_edge_var, logic::Or(ctx, nev)));
                } else {
                    out.push_back((metal_edge_var == ctx.bool_val(false)));
                }
            }
        out.push_back(logic::eq_expr(ctx, metal_point_var, logic::Or(ctx, incident_metal_edges)));
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_minimum_pin_length(
    const Config& cfg, const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points, const RoutingGraph& g, const std::vector<Net>& nets) {
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    const long min_pin_len = cfg.opt_long("minimum_pin_length", 0);
    std::vector<z3::expr> out;
    for (const NetInfo& ni : infos) {
        if (ni.is_power || !ni.is_ext_pin) continue;
        int ext_idx_pin = -1;
        for (int i = 0; i < (int)ni.net->pins.size(); ++i)
            if (ni.net->pins[i].term == TERM_EXT_PIN) { ext_idx_pin = i; break; }
        if (ext_idx_pin < 0) continue;
        const int outer_pin_comm = ext_idx_pin - 1;
        const std::vector<Point>& outer_points = ni.net->pins[ext_idx_pin].points;
        std::set<Arr> outer_set;
        std::set<long> outer_pin_ys;
        long min_x = std::numeric_limits<long>::max(), max_x = 0;
        for (const Point& p : outer_points) {
            outer_set.insert({p.x, p.y, p.z});
            outer_pin_ys.insert(p.y);
            min_x = std::min(min_x, p.x); max_x = std::max(max_x, p.x);
        }
        auto adj = net_adjacency(ni, g);
        const std::string& ext_layer0 = cfg.ext_pin_layer.empty() ? std::string() : cfg.ext_pin_layer[0];

        for (const std::string& epl : cfg.ext_pin_layer) {
            int z = cfg.routing_layer_index(epl);
            if (z < 0) continue;
            const int routing_dir = cfg.routing_directions[z];
            const std::string& layer = cfg.routing_layers[z];
            if (min_pin_len <= cfg.width(layer)) break;
            const long layer_ext = (routing_dir == VERTICAL) ? cfg.rules.at("extension").at(ext_layer0).get<long>()
                                                             : cfg.width(ext_layer0);
            const auto& ys = y_points[z];
            const long final_y = ys.empty() ? 0 : ys.back();
            for (long x : x_points[z]) {
                if (!(min_x <= x && x <= max_x)) continue;
                for (long outer_pin_y : outer_pin_ys) {
                    int oy_index = -1;
                    for (int i = 0; i < (int)ys.size(); ++i) if (ys[i] == outer_pin_y) { oy_index = i; break; }
                    if (oy_index < 0) continue;
                    std::vector<logic::Cond> y_pin_edge_vars;
                    for (int y_index = oy_index; y_index >= 0; --y_index) {
                        const long curr_y = ys[y_index];
                        if ((final_y - curr_y) + layer_ext < min_pin_len) break;
                        std::vector<std::pair<Arr, Arr>> pin_edges;
                        Arr curr_point{x, curr_y, z};
                        for (int i = y_index + 1; i < (int)ys.size(); ++i) {
                            const long next_y = ys[i];
                            const Arr next_point{x, next_y, z};
                            if (!(edge_in_graph(g, curr_point, next_point) && in_bbox(ni, curr_point) && in_bbox(ni, next_point)))
                                continue;
                            pin_edges.emplace_back(curr_point, next_point);
                            curr_point = next_point;
                            if (min_pin_len <= (next_y - curr_y) + layer_ext) {
                                bool touches = false;
                                for (auto& e : pin_edges) if (e.first[1] == outer_pin_y || e.second[1] == outer_pin_y) { touches = true; break; }
                                if (touches) break; else continue;
                            }
                        }
                        if (!pin_edges.empty()) {
                            std::vector<logic::Cond> evs;
                            for (auto& e : pin_edges) evs.push_back(logic::Cond::Expr(bool_var(net_edge_var_name(ni.cname, e.first, e.second))));
                            logic::Cond chain = logic::And(ctx, evs);
                            z3::expr ext_pin_var = bool_var(ext_pin_chain_var_name(ni.cname, x, curr_y, z));
                            out.push_back(logic::eq_expr(ctx, ext_pin_var, chain));
                            y_pin_edge_vars.push_back(chain);
                        }
                    }
                    logic::Cond outer_pin = logic::Or(ctx, y_pin_edge_vars);
                    const Arr pin_point{x, outer_pin_y, z};
                    auto it = adj.find(pin_point);
                    if (it == adj.end()) continue;
                    for (const Arr& q : it->second) {
                        if (outer_set.count(q)) continue;
                        z3::expr be = bool_var(comm_edge_var_name(ni.cname, outer_pin_comm, pin_point, q));
                        out.push_back(*logic::Or(ctx, {logic::Not(logic::Cond::Expr(be)), outer_pin}).e);
                    }
                }
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_commodity_flow(const Config& cfg, const RoutingGraph& g,
                                                      const std::vector<Net>& nets) {
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    std::vector<z3::expr> out;
    for (const NetInfo& ni : infos) {
        if (ni.is_dummy || ni.is_power) continue;
        const int pin_count = (int)ni.net->pins.size();
        if (pin_count <= 1) continue;
        const int commodity_count = pin_count - 1;
        auto adj = net_adjacency(ni, g);
        std::vector<std::set<Arr>> pin_sets(pin_count);
        for (int i = 0; i < pin_count; ++i)
            for (const Point& p : ni.net->pins[i].points) pin_sets[i].insert({p.x, p.y, p.z});
        const std::set<Arr>& source = pin_sets[0];

        auto comm = [&](int c, const Arr& a, const Arr& b) {
            return bool_var(comm_edge_var_name(ni.cname, c, a, b));
        };
        auto boundary = [&](int c, int pin_index) {
            std::vector<z3::expr> b;
            for (const Point& pp : ni.net->pins[pin_index].points) {
                const Arr p{pp.x, pp.y, pp.z};
                auto it = adj.find(p);
                if (it == adj.end()) continue;
                for (const Arr& q : it->second)
                    if (!pin_sets[pin_index].count(q)) b.push_back(comm(c, p, q));
            }
            return b;
        };

        for (const auto& kv : adj) {
            const Arr& point = kv.first;
            for (int c = 0; c < commodity_count; ++c) {
                if (source.count(point) || pin_sets[c + 1].count(point)) continue;
                std::vector<z3::expr> bvs;
                for (const Arr& q : kv.second) bvs.push_back(comm(c, point, q));
                if (bvs.size() == 1) out.push_back((bvs[0] == ctx.bool_val(false)));
                else if (bvs.size() == 2) out.push_back((!(bvs[0] ^ bvs[1])));
                else if (bvs.size() > 2) out.push_back(not_exactly_one(ctx, bvs));
            }
        }
        for (int c = 0; c < commodity_count; ++c) {
            auto bvs = boundary(c, 0);
            if (bvs.empty()) continue;
            if (bvs.size() == 1) out.push_back((bvs[0] == ctx.bool_val(true)));
            else out.push_back(exactly(ctx, bvs, 1));
        }
        for (int pin_index = 1; pin_index < pin_count; ++pin_index) {
            int c = pin_index - 1;
            auto bvs = boundary(c, pin_index);
            if (bvs.empty()) continue;
            if (bvs.size() == 1) out.push_back((bvs[0] == ctx.bool_val(true)));
            else out.push_back(exactly(ctx, bvs, 1));
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_cuts(const Config& cfg, const RoutingGraph& g,
                                            const std::vector<Net>& nets) {
    std::vector<z3::expr> out;
    const bool PIN_EDGE = cfg.opt_bool("pin_edge_lower_bound_cut");
    if (!PIN_EDGE) return out;
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    for (const NetInfo& ni : infos) {
        if (ni.is_dummy || ni.is_power) continue;
        const int pin_count = (int)ni.net->pins.size();
        if (pin_count <= 1) continue;
        auto adj = net_adjacency(ni, g);
        std::vector<std::set<Arr>> pin_sets(pin_count);
        for (int i = 0; i < pin_count; ++i)
            for (const Point& p : ni.net->pins[i].points) pin_sets[i].insert({p.x, p.y, p.z});

        if (PIN_EDGE) {
            const int n_pin = cfg.opt_bool("pin_edge_lb_source_only") ? 1 : pin_count;
            for (int pi = 0; pi < n_pin; ++pi) {
                std::set<std::string> seen;
                std::vector<z3::expr> base;
                for (const Point& pp : ni.net->pins[pi].points) {
                    const Arr p{pp.x, pp.y, pp.z};
                    auto it = adj.find(p);
                    if (it == adj.end()) continue;
                    for (const Arr& q : it->second)
                        if (!pin_sets[pi].count(q)) {
                            std::string nm = edge_var_name(p, q);
                            if (seen.insert(nm).second) base.push_back(bool_var(nm));
                        }
                }
                if (!base.empty()) out.push_back(at_least(ctx, base, 1));
            }
        }

    }
    return out;
}

std::vector<z3::expr> SmtModel::add_pin_grid_lock(const Config& cfg, const RoutingGraph& g,
                                                     const std::vector<Net>& nets) {
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    std::vector<z3::expr> out;
    for (const NetInfo& ni : infos) {
        for (const Pin& pin : ni.net->pins) {
            if (pin.term != TERM_DRAIN && pin.term != TERM_SOURCE && pin.term != TERM_GATE) continue;
            for (const Point& p : pin.points) {
                const Arr a{p.x, p.y, p.z};
                if (!g.nodes.count(a) || !in_bbox(ni, a)) continue;
                if (ni.is_dummy && !ni.pinset.count(a)) continue;
                z3::expr nv = bool_var(net_point_var_name(ni.cname, p.x, p.y, p.z));
                z3::expr base = bool_var(point_var_name("G", p.x, p.y, p.z));
                out.push_back((nv == base));
            }
        }
    }
    return out;
}

// Each port net must own a drawn Metal1 edge holding a P&R pin grid point, joined by drawn edges of the net to one of
// its device pins, which is the metal the GDS writer searches when it places the port's label and pin shape.
std::vector<z3::expr> SmtModel::add_pin_grid_witness(const Config& cfg, const RoutingGraph& g,
                                                        const std::vector<Net>& nets) {
    std::vector<z3::expr> out;
    if (!pin_grid_active(cfg) || cfg.ext_pin_layer.empty()) return out;
    pin_grid_lits.clear();
    const long m1 = cfg.routing_layer_index(cfg.ext_pin_layer.back());
    auto infos = build_net_infos(cfg, nets, tolerance, cfg.x_unit, cfg.y_unit);
    for (const NetInfo& ni : infos) {
        if (ni.is_dummy || ni.is_power || !ni.is_ext_pin) continue;
        if ((int)ni.net->pins.size() <= 1) continue;
        std::set<Arr> roots;
        for (const Pin& p : ni.net->pins) {
            if (p.term == TERM_EXT_PIN || p.term == TERM_ACCESS_POINT) continue;
            for (const Point& q : p.points) roots.insert({q.x, q.y, q.z});
        }
        auto adj = net_adjacency(ni, g);
        auto hit = [&](const Arr& a, const Arr& b) {
            long px = 0, py = 0;
            return pin_grid_hit(cfg, pin_shift, a[0], a[1], b[0], b[1], px, py);
        };
        const std::string pre = "PGR_" + ni.cname + "_";
        auto key = [&](const Arr& p) {
            return pre + std::to_string(p[0]) + "_" + std::to_string(p[1]) + "_" + std::to_string(p[2]);
        };
        auto reach = [&](const Arr& p) { return roots.count(p) ? ctx.bool_val(true) : bool_var("R" + key(p)); };
        auto lab = [&](const Arr& p) { return roots.count(p) ? ctx.int_val(0) : ctx.int_const(("L" + key(p)).c_str()); };
        auto drawn = [&](const Arr& a, const Arr& b) { return bool_var(net_edge_var_name(ni.cname, a, b)); };
        const int top = (int)adj.size() + 1;
        // A node is reached only through a drawn edge of this net from a reached node with a smaller label, so
        // reached metal hangs off a device pin and a loop standing apart from the net never counts.
        for (const auto& kv : adj) {
            const Arr& n = kv.first;
            if (roots.count(n)) continue;
            const z3::expr ln = lab(n);
            z3::expr_vector from(ctx);
            for (const Arr& q : kv.second) from.push_back(drawn(n, q) && reach(q) && lab(q) < ln);
            out.push_back(ln >= 1 && ln <= top);
            out.push_back(z3::implies(reach(n), from.empty() ? ctx.bool_val(false) : z3::mk_or(from)));
        }
        // The witness: a drawn Metal1 edge of this net on a grid node or spanning a grid point, at a reached node.
        std::set<std::string> seen;
        z3::expr_vector lits(ctx);
        for (const auto& kv : adj) {
            const Arr& a = kv.first;
            if (a[2] != m1) continue;
            const bool node = hit(a, a);
            for (const Arr& b : kv.second) {
                if (!(node || (b[2] == m1 && (hit(b, b) || hit(a, b))))) continue;
                if (!seen.insert(net_edge_var_name(ni.cname, a, b) + "@" + key(a)).second) continue;
                const z3::expr w = drawn(a, b) && reach(a);
                lits.push_back(w);
                pin_grid_lits.push_back({ni.net->name, w, a, b});
            }
        }
        out.push_back(lits.empty() ? ctx.bool_val(false) : z3::mk_or(lits));
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_pre_layout_blockage(
    const Config& cfg, const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points, const RoutingGraph& g, const PreLayout& pre) {
    static const char* SIDE_P[4] = {"SIDE_L", "SIDE_R", "SIDE_T", "SIDE_B"};
    static const char* TIP_P[4] = {"TIP_L", "TIP_R", "TIP_T", "TIP_B"};
    static const char* CORNER_P[4] = {"CORNER_TL", "CORNER_TR", "CORNER_BL", "CORNER_BR"};
    const int QUERY_SIDE = 0, QUERY_TIP = 1, QUERY_CORNER = 2;
    const int ENC_HORIZONTAL = 0, ENC_VERTICAL = 1;

    const auto& lmj = cfg.specs.at("layer_map");
    std::set<std::string> available;
    for (auto it = lmj.begin(); it != lmj.end(); ++it) {
        int num = it.value().get<int>();
        auto rit = pre.rectangles.find(num);
        if (rit != pre.rectangles.end() && !rit->second.empty()) available.insert(it.key());
    }
    std::vector<std::string> next_layers;
    for (auto it = lmj.begin(); it != lmj.end(); ++it) next_layers.push_back(it.key());

    const int nlayers = (int)cfg.routing_layers.size();
    std::map<int, std::vector<Arr>> layer_pts, via_pts;
    for (const auto& n : g.nodes) layer_pts[(int)n[2]].push_back(n);
    for (const Arr& vp : compute_via_points_net(cfg, x_points, y_points, g)) via_pts[(int)vp[2]].push_back(vp);
    std::vector<std::map<long, int>> xidx(nlayers), yidx(nlayers);
    for (int z = 0; z < nlayers; ++z) {
        for (int i = 0; i < (int)x_points[z].size(); ++i) xidx[z][x_points[z][i]] = i;
        for (int i = 0; i < (int)y_points[z].size(); ++i) yidx[z][y_points[z][i]] = i;
    }

    std::vector<z3::expr> out;
    auto edge_or_none = [&](const std::array<std::array<long, 3>, 2>& e) -> std::optional<z3::expr> {
        if (!edge_in_graph(g, e[0], e[1])) return std::nullopt;
        return bool_var(edge_var_name(e[0], e[1]));
    };
    auto emit = [&](z3::expr point_val, const std::optional<std::array<std::array<long, 3>, 2>>& not_edge) {
        if (!not_edge) { out.push_back((point_val == ctx.bool_val(false))); return; }
        auto nev = edge_or_none(*not_edge);
        if (nev) out.push_back(((*nev) || (!point_val)));
        else out.push_back((point_val == ctx.bool_val(false)));
    };

    const int total = nlayers + (int)cfg.vias.size();
    for (int zz = 0; zz < total; ++zz) {
        const bool is_layer = zz < nlayers;
        const std::string curr_layer = is_layer ? cfg.routing_layers[zz] : cfg.vias[zz - nlayers];
        const int routing_dir = is_layer ? cfg.routing_directions[zz] : 2;
        const int z = is_layer ? zz : zz - nlayers;

        for (const std::string& next_layer : next_layers) {
            if (!available.count(next_layer)) continue;
            double s2s = spacing0(cfg, "S2S", curr_layer, next_layer);
            double s2t = spacing0(cfg, "S2T", curr_layer, next_layer);
            double t2t = spacing0(cfg, "T2T", curr_layer, next_layer);
            double c2c = spacing0(cfg, "C2C", curr_layer, next_layer);
            if (s2s < 0 && s2t < 0 && t2t < 0 && c2c < 0) continue;
            const bool run_s2s = s2s >= 0, run_s2t = s2t >= 0, run_t2t = t2t >= 0, run_c2c = c2c >= 0;

            const auto& pts = is_layer ? layer_pts[z] : via_pts[z];
            for (const Arr& p : pts) {
                const long x = p[0], y = p[1];
                if (!xidx[z].count(x) || !yidx[z].count(y)) continue;
                const int xi = xidx[z][x], yi = yidx[z][y];
                const double x_ex = (routing_dir == ENC_HORIZONTAL) ? cfg.rules.at("extension").at(curr_layer).get<double>() : cfg.width(curr_layer);
                const double y_ex = (routing_dir == ENC_VERTICAL) ? cfg.rules.at("extension").at(curr_layer).get<double>() : cfg.width(curr_layer);

                auto pv = [&](const char* prefix) {
                    return is_layer ? bool_var(point_var_name(prefix, x, y, z))
                                    : bool_var(point_var_name("VIA", x, y, z));
                };
                if (run_s2s) {
                    auto q = pre.query_polygon(cfg, x_points[z], y_points[z], xi, yi, x_ex, y_ex, next_layer, z, QUERY_SIDE, s2s);
                    for (int d = 0; d < 4; ++d) if (q.dir[d]) emit(pv(SIDE_P[d]), q.not_edge);
                }
                if (run_s2t) {
                    auto q = pre.query_polygon(cfg, x_points[z], y_points[z], xi, yi, x_ex, y_ex, next_layer, z, QUERY_TIP, s2t);
                    for (int d = 0; d < 4; ++d) if (q.dir[d]) emit(pv(SIDE_P[d]), q.not_edge);
                    auto q2 = pre.query_polygon(cfg, x_points[z], y_points[z], xi, yi, x_ex, y_ex, next_layer, z, QUERY_SIDE, s2t);
                    for (int d = 0; d < 4; ++d) if (q2.dir[d]) emit(pv(TIP_P[d]), q2.not_edge);
                }
                if (run_t2t) {
                    auto q = pre.query_polygon(cfg, x_points[z], y_points[z], xi, yi, x_ex, y_ex, next_layer, z, QUERY_TIP, t2t);
                    for (int d = 0; d < 4; ++d) if (q.dir[d]) emit(pv(TIP_P[d]), q.not_edge);
                }
                if (run_c2c) {
                    auto q = pre.query_polygon(cfg, x_points[z], y_points[z], xi, yi, x_ex, y_ex, next_layer, z, QUERY_CORNER, c2c);
                    for (int d = 0; d < 4; ++d)
                        if (q.dir[d]) out.push_back((pv(CORNER_P[d]) == ctx.bool_val(false)));
                }
                if (is_layer && cfg.opt_bool("enable_via_enc_blockage")) {
                    static const char* ENC_P[2][2] = {{"VIA_ENC_HOR_L", "VIA_ENC_HOR_U"},
                                                      {"VIA_ENC_VER_L", "VIA_ENC_VER_U"}};
                    for (int enc_dir = 0; enc_dir < 2; ++enc_dir)
                        for (int level = 0; level < 2; ++level) {
                            const auto& via_opt = (level == 0) ? cfg.lower_via.at(curr_layer) : cfg.upper_via.at(curr_layer);
                            if (!via_opt.has_value() || via_opt->empty()) continue;
                            const std::string& via = *via_opt;
                            long via_enc_len = 0;
                            if (cfg.rules.at("enclosure").contains(via) && cfg.rules.at("enclosure").at(via).contains(curr_layer))
                                via_enc_len = cfg.rules.at("enclosure").at(via).at(curr_layer).get<long>();
                            const long via_width = cfg.width(via), metal_width = cfg.width(curr_layer);
                            const long metal_ex2 = cfg.rules.at("extension").at(curr_layer).get<long>();
                            const long along_ex = (via_enc_len != 0) ? (via_width + 2 * via_enc_len) : metal_ex2;
                            const double enc_x_ex = (enc_dir == ENC_HORIZONTAL) ? (double)along_ex : (double)metal_width;
                            const double enc_y_ex = (enc_dir == ENC_HORIZONTAL) ? (double)metal_width : (double)along_ex;
                            z3::expr enc_var = bool_var(point_var_name(ENC_P[enc_dir][level], x, y, z));
                            const std::pair<int, double> modes[2] = {{QUERY_SIDE, s2s}, {QUERY_TIP, s2t}};
                            const bool runs[2] = {run_s2s, run_s2t};
                            for (int mi = 0; mi < 2; ++mi) {
                                if (!runs[mi]) continue;
                                auto q = pre.query_polygon(cfg, x_points[z], y_points[z], xi, yi, enc_x_ex, enc_y_ex,
                                                           next_layer, z, modes[mi].first, modes[mi].second);
                                bool any = false;
                                for (int d = 0; d < 4; ++d) if (q.dir[d]) { any = true; break; }
                                if (any) { emit(enc_var, q.not_edge); break; }
                            }
                        }
                }
            }
        }
    }
    return out;
}

}
