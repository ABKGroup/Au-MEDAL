// Geometric helper constraints: metal direction, tip/side/corner/via
// variables and their spacing prerequisites.
#include "smt.hpp"
#include <algorithm>
#include <optional>
#include "logic.hpp"
#include "variables.hpp"

namespace aumedal {
using logic::Cond;

namespace {
std::map<std::string, int> build_layer_index(const Config& cfg) {
    std::map<std::string, int> idx;
    for (int i = 0; i < (int)cfg.routing_layers.size(); ++i) idx[cfg.routing_layers[i]] = i;
    return idx;
}
}

std::vector<z3::expr> SmtModel::tip_side_exclusivity(const std::set<std::array<long, 3>>& nodes) {
    static const std::vector<std::pair<std::string, std::string>> dirs = {
        {"SIDE_T", "TIP_T"}, {"SIDE_B", "TIP_B"}, {"SIDE_R", "TIP_R"}, {"SIDE_L", "TIP_L"}};
    std::vector<z3::expr> out;
    for (const auto& n : nodes) {
        for (const auto& d : dirs) {
            z3::expr side = bool_var(point_var_name(d.first, n[0], n[1], n[2]));
            z3::expr tip = bool_var(point_var_name(d.second, n[0], n[1], n[2]));
            z3::expr c = !(side && tip);
            out.push_back(c);
        }
    }
    return out;
}

namespace {
z3::expr or_exprs(const std::vector<z3::expr>& es) {
    if (es.size() == 1) return es[0];
    z3::expr_vector v(es[0].ctx());
    for (auto& e : es) v.push_back(e);
    return z3::mk_or(v);
}
z3::expr equal_expr(const z3::expr& a, const z3::expr& b) {
    return (!a || b) && (!b || a);
}
std::optional<long> getpos(const std::vector<long>& v, long idx) {
    if (idx >= 0 && idx < (long)v.size()) return v[idx];
    return std::nullopt;
}
}

std::vector<z3::expr> SmtModel::via_helper(const Config& cfg,
                                              const std::vector<std::vector<long>>& x_points,
                                              const std::vector<std::vector<long>>& y_points,
                                              const RoutingGraph& g) {
    std::map<std::string, int> layer_index = build_layer_index(cfg);
    std::map<std::string, int> via_index;
    for (int i = 0; i < (int)cfg.vias.size(); ++i) via_index[cfg.vias[i]] = i;

    std::map<std::array<long, 3>, std::vector<std::string>> via_upper_edges;
    std::vector<z3::expr> out;

    for (int z = 0; z < (int)cfg.routing_layers.size(); ++z) {
        const std::string& layer = cfg.routing_layers[z];
        std::vector<int> upper_zs;
        for (auto& ul : cfg.upper_layers.at(layer))
            if (layer_index.count(ul)) upper_zs.push_back(layer_index[ul]);
        int max_same = -1;
        for (auto& sh : cfg.same_height_layers.at(layer))
            if (layer_index.count(sh)) max_same = std::max(max_same, layer_index[sh]);
        auto uv = cfg.upper_via.find(layer);
        if (uv == cfg.upper_via.end() || !uv->second.has_value()) continue;
        auto vit = via_index.find(*uv->second);
        if (vit == via_index.end()) continue;
        const int via_idx = vit->second;

        for (long x : x_points[z]) {
            for (long y : y_points[z]) {
                const std::array<long, 3> point{x, y, z};
                const std::array<long, 3> via_key{x, y, via_idx};
                for (int uz : upper_zs) {
                    const std::array<long, 3> up{x, y, uz};
                    if (edge_in_graph(g,point, up))
                        via_upper_edges[via_key].push_back(edge_var_name(point, up));
                }
                if (z == max_same) {
                    auto it = via_upper_edges.find(via_key);
                    if (it != via_upper_edges.end() && !it->second.empty()) {
                        std::vector<z3::expr> es;
                        for (auto& nm : it->second) es.push_back(bool_var(nm));
                        z3::expr via_var = bool_var(point_var_name("VIA", x, y, via_idx));
                        out.push_back(equal_expr(or_exprs(es), via_var));
                    }
                }
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::metal_direction(
    const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points,
    const RoutingGraph& g) {
    std::vector<z3::expr> out;
    const int nz = static_cast<int>(x_points.size());
    for (int z = 0; z < nz; ++z) {
        const auto& xs = x_points[z];
        const auto& ys = y_points[z];
        for (size_t xi = 0; xi < xs.size(); ++xi) {
            const long x = xs[xi];
            const bool hasL = xi > 0, hasR = xi + 1 < xs.size();
            const long lx = hasL ? xs[xi - 1] : 0, rx = hasR ? xs[xi + 1] : 0;
            for (size_t yi = 0; yi < ys.size(); ++yi) {
                const long y = ys[yi];
                const bool hasB = yi > 0, hasT = yi + 1 < ys.size();
                const long by = hasB ? ys[yi - 1] : 0, ty = hasT ? ys[yi + 1] : 0;
                const std::array<long, 3> node{x, y, z};
                if (!g.nodes.count(node)) continue;

                std::vector<z3::expr> ver, hor;
                if (hasT && edge_in_graph(g,node, {x, ty, z})) ver.push_back(bool_var(edge_var_name(node, {x, ty, z})));
                if (hasB && edge_in_graph(g,node, {x, by, z})) ver.push_back(bool_var(edge_var_name(node, {x, by, z})));
                if (hasR && edge_in_graph(g,node, {rx, y, z})) hor.push_back(bool_var(edge_var_name(node, {rx, y, z})));
                if (hasL && edge_in_graph(g,node, {lx, y, z})) hor.push_back(bool_var(edge_var_name(node, {lx, y, z})));

                z3::expr ver_e = ver.empty() ? ctx.bool_val(false) : or_exprs(ver);
                z3::expr hor_e = hor.empty() ? ctx.bool_val(false) : or_exprs(hor);
                out.push_back(bool_var(point_var_name("METAL_VER", x, y, z)) == ver_e);
                out.push_back(bool_var(point_var_name("METAL_HOR", x, y, z)) == hor_e);
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::tip_helper(
    const Config& cfg,
    const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points,
    const RoutingGraph& g) {
    using Arr = std::array<long, 3>;
    using OArr = std::optional<Arr>;
    using OL = std::optional<long>;

    auto E = [&](const OArr& a, const OArr& b) -> Cond {
        if (!a || !b) return Cond::None();
        if (!edge_in_graph(g,*a, *b)) return Cond::None();
        return Cond::Expr(bool_var(edge_var_name(*a, *b)));
    };
    auto with_edges = points_with_edges(g);

    std::map<std::string, int> layer_index = build_layer_index(cfg);

    std::vector<z3::expr> out;
    const int nz = (int)cfg.routing_layers.size();
    for (int z = 0; z < nz; ++z) {
        const std::string& layer = cfg.routing_layers[z];
        std::vector<int> lower_zs, upper_zs, same_height_zs;
        for (auto& l : cfg.lower_layers.at(layer)) if (layer_index.count(l)) lower_zs.push_back(layer_index[l]);
        for (auto& l : cfg.upper_layers.at(layer)) if (layer_index.count(l)) upper_zs.push_back(layer_index[l]);
        for (auto& l : cfg.same_height_layers.at(layer)) if (layer_index.count(l)) same_height_zs.push_back(layer_index[l]);
        const long layer_width = cfg.width(layer);
        const long max_tip_len = cfg.rules.at("max_tip_len").at(layer).get<long>();
        const int routing_dir = cfg.routing_directions[z];
        const auto& xs = x_points[z];
        const auto& ys = y_points[z];

        for (size_t xi = 0; xi < xs.size(); ++xi) {
            const long x = xs[xi];
            for (size_t yi = 0; yi < ys.size(); ++yi) {
                const long y = ys[yi];
                const Arr point{x, y, z};
                if (!with_edges.count(point)) continue;

                const OL bottom_y = getpos(ys, (long)yi - 1), top_y = getpos(ys, (long)yi + 1);
                const OL right_x = getpos(xs, (long)xi + 1), left_x = getpos(xs, (long)xi - 1);

                std::vector<Cond> top_conditions, bottom_conditions, right_conditions, left_conditions;

                auto pt = [&](OL px, OL py) -> OArr {
                    if (!px || !py) return std::nullopt;
                    return Arr{*px, *py, z};
                };
                const OArr right_point = pt(right_x, y), left_point = pt(left_x, y);
                const OArr top_point = pt(x, top_y), bottom_point = pt(x, bottom_y);

                const Cond right_e = E(point, right_point), left_e = E(point, left_point);
                const Cond top_e = E(point, top_point), bottom_e = E(point, bottom_point);

                std::vector<Cond> same_e, lower_e, upper_e;
                for (int shz : same_height_zs) { Cond c = E(point, Arr{x, y, shz}); if (c.kind != Cond::NONE) same_e.push_back(c); }
                for (int lz : lower_zs) { Cond c = E(point, Arr{x, y, lz}); if (c.kind != Cond::NONE) lower_e.push_back(c); }
                for (int uz : upper_zs) { Cond c = E(point, Arr{x, y, uz}); if (c.kind != Cond::NONE) upper_e.push_back(c); }

                auto cat = [&](const Cond& head, const std::vector<Cond>& a, const std::vector<Cond>& b,
                               const std::vector<Cond>& c) {
                    std::vector<Cond> v{head};
                    v.insert(v.end(), a.begin(), a.end());
                    v.insert(v.end(), b.begin(), b.end());
                    v.insert(v.end(), c.begin(), c.end());
                    return v;
                };

                if (layer_width <= max_tip_len) {
                    left_conditions.push_back(logic::And(ctx, {logic::Or(ctx, cat(right_e, upper_e, lower_e, same_e)),
                                                               logic::Not(logic::Or(ctx, {left_e, top_e, bottom_e}))}));
                    right_conditions.push_back(logic::And(ctx, {logic::Or(ctx, cat(left_e, upper_e, lower_e, same_e)),
                                                                logic::Not(logic::Or(ctx, {right_e, top_e, bottom_e}))}));
                    top_conditions.push_back(logic::And(ctx, {logic::Or(ctx, cat(bottom_e, upper_e, lower_e, same_e)),
                                                              logic::Not(logic::Or(ctx, {top_e, left_e, right_e}))}));
                    bottom_conditions.push_back(logic::And(ctx, {logic::Or(ctx, cat(top_e, upper_e, lower_e, same_e)),
                                                                 logic::Not(logic::Or(ctx, {bottom_e, left_e, right_e}))}));
                }

                for (int dir : {HORIZONTAL, VERTICAL}) {
                    const bool is_horizontal_dir = (dir == HORIZONTAL);
                    const std::vector<long>& main_pts = is_horizontal_dir ? xs : ys;
                    const long main_index = is_horizontal_dir ? (long)xi : (long)yi;
                    const long main_pos = is_horizontal_dir ? x : y;
                    const long sub_pos = is_horizontal_dir ? y : x;
                    const OL sub_next_pos = is_horizontal_dir ? top_y : right_x;
                    const OL sub_prev_pos = is_horizontal_dir ? bottom_y : left_x;
                    auto mk = [&](OL mainc, OL crossc) -> OArr {
                        if (!mainc || !crossc) return std::nullopt;
                        return is_horizontal_dir ? Arr{*mainc, *crossc, z} : Arr{*crossc, *mainc, z};
                    };
                    const Cond sub_next_edge_var = E(point, mk(main_pos, sub_next_pos));
                    const Cond sub_prev_edge_var = E(point, mk(main_pos, sub_prev_pos));
                    std::vector<Cond>& next_conds = is_horizontal_dir ? top_conditions : right_conditions;
                    std::vector<Cond>& prev_conds = is_horizontal_dir ? bottom_conditions : left_conditions;

                    for (long si = main_index; si >= 0; --si) {
                        const long s_pos = main_pts[si];
                        const OArr s_point = mk(s_pos, sub_pos);
                        long length = main_pos - s_pos + layer_width;
                        if (max_tip_len < length - layer_width) break;

                        const Cond s_sub_next = E(s_point, mk(s_pos, sub_next_pos));
                        const Cond s_sub_prev = E(s_point, mk(s_pos, sub_prev_pos));
                        std::vector<Cond> sub_next_edges{s_sub_next};
                        std::vector<Cond> sub_prev_edges{s_sub_prev};
                        std::vector<Cond> target_edges;

                        for (long li = si + 1; li < (long)main_pts.size(); ++li) {
                            const long l_pos = main_pts[li];
                            const OArr l_point = mk(l_pos, sub_pos);
                            const Cond l_sub_next = E(l_point, mk(l_pos, sub_next_pos));
                            const Cond l_sub_prev = E(l_point, mk(l_pos, sub_prev_pos));
                            sub_next_edges.push_back(l_sub_next);
                            sub_prev_edges.push_back(l_sub_prev);

                            const OL prev_s_pos = getpos(main_pts, si - 1);
                            const OArr prev_s_point = mk(prev_s_pos, sub_pos);
                            const OL next_l_pos = getpos(main_pts, li + 1);
                            const OArr next_l_point = mk(next_l_pos, sub_pos);
                            const Cond prev_smallest_edge = E(prev_s_point, s_point);
                            const Cond next_largest_edge = E(l_point, next_l_point);

                            const OL prev_l_pos = getpos(main_pts, li - 1);
                            const OArr prev_l_point = mk(prev_l_pos, sub_pos);
                            const Cond link_edge = E(prev_l_point, l_point);
                            if (link_edge.kind == Cond::NONE) break;
                            target_edges.push_back(link_edge);

                            if (l_pos < main_pos) continue;
                            length = (l_pos - s_pos) + layer_width;

                            if (length <= max_tip_len) {
                                std::vector<Cond> nx = sub_next_edges;
                                nx.push_back(prev_smallest_edge);
                                nx.push_back(next_largest_edge);
                                next_conds.push_back(logic::And(ctx, {logic::And(ctx, target_edges),
                                                                      logic::Not(logic::Or(ctx, nx))}));
                                std::vector<Cond> pv = sub_prev_edges;
                                pv.push_back(prev_smallest_edge);
                                pv.push_back(next_largest_edge);
                                prev_conds.push_back(logic::And(ctx, {logic::And(ctx, target_edges),
                                                                      logic::Not(logic::Or(ctx, pv))}));
                            }

                            if (routing_dir == BIDIRECTION) {
                                const long lmw = length - layer_width;
                                const long lm2w = length - 2 * layer_width;
                                if (0 < lmw && lmw <= max_tip_len) {
                                    if (main_pos != s_pos) {
                                        if (s_sub_next.kind != Cond::NONE)
                                            next_conds.push_back(logic::And(ctx, {logic::And(ctx, target_edges), s_sub_next,
                                                logic::Not(logic::Or(ctx, {sub_next_edge_var, next_largest_edge}))}));
                                        if (s_sub_prev.kind != Cond::NONE)
                                            prev_conds.push_back(logic::And(ctx, {logic::And(ctx, target_edges), s_sub_prev,
                                                logic::Not(logic::Or(ctx, {sub_prev_edge_var, next_largest_edge}))}));
                                    }
                                    if (main_pos != l_pos) {
                                        if (l_sub_next.kind != Cond::NONE)
                                            next_conds.push_back(logic::And(ctx, {logic::And(ctx, target_edges), l_sub_next,
                                                logic::Not(logic::Or(ctx, {sub_next_edge_var, prev_smallest_edge}))}));
                                        if (l_sub_prev.kind != Cond::NONE)
                                            prev_conds.push_back(logic::And(ctx, {logic::And(ctx, target_edges), l_sub_prev,
                                                logic::Not(logic::Or(ctx, {sub_prev_edge_var, prev_smallest_edge}))}));
                                    }
                                } else if (0 < lmw && lm2w <= max_tip_len) {
                                    if (main_pos != s_pos && main_pos != l_pos &&
                                        s_sub_next.kind != Cond::NONE && l_sub_next.kind != Cond::NONE &&
                                        s_sub_prev.kind != Cond::NONE && l_sub_prev.kind != Cond::NONE) {
                                        next_conds.push_back(logic::And(ctx, {logic::And(ctx, target_edges), s_sub_next, l_sub_next,
                                            logic::Not(logic::Or(ctx, {sub_next_edge_var}))}));
                                        prev_conds.push_back(logic::And(ctx, {logic::And(ctx, target_edges), s_sub_prev, l_sub_prev,
                                            logic::Not(logic::Or(ctx, {sub_prev_edge_var}))}));
                                    }
                                } else {
                                    break;
                                }
                            }
                        }
                    }
                }

                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("TIP_L", x, y, z)), logic::Or(ctx, left_conditions)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("TIP_R", x, y, z)), logic::Or(ctx, right_conditions)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("TIP_T", x, y, z)), logic::Or(ctx, top_conditions)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("TIP_B", x, y, z)), logic::Or(ctx, bottom_conditions)));
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::side_helper(
    const Config& cfg,
    const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points,
    const RoutingGraph& g) {
    using Arr = std::array<long, 3>;
    using OArr = std::optional<Arr>;
    using OL = std::optional<long>;
    using OS = std::optional<std::string>;

    auto ename = [&](const OArr& a, const OArr& b) -> OS {
        if (!a || !b || !edge_in_graph(g,*a, *b)) return std::nullopt;
        return edge_var_name(*a, *b);
    };
    auto to_conds = [&](const std::set<std::string>& s) {
        std::vector<Cond> v;
        for (const auto& n : s) v.push_back(Cond::Expr(bool_var(n)));
        return v;
    };
    auto with_edges = points_with_edges(g);

    std::vector<z3::expr> out;
    const int nz = (int)cfg.routing_layers.size();
    for (int z = 0; z < nz; ++z) {
        const std::string& layer = cfg.routing_layers[z];
        const int routing_dir = cfg.routing_directions[z];
        const long layer_width = cfg.width(layer);
        const long extension = cfg.rules.at("extension").at(layer).get<long>();
        const long max_tip_len = cfg.rules.at("max_tip_len").at(layer).get<long>();
        const long width_or_ext = (routing_dir == BIDIRECTION) ? layer_width : extension;
        const auto& xs = x_points[z];
        const auto& ys = y_points[z];

        for (size_t xi = 0; xi < xs.size(); ++xi) {
            const long x = xs[xi];
            for (size_t yi = 0; yi < ys.size(); ++yi) {
                const long y = ys[yi];
                const Arr point{x, y, z};
                if (!with_edges.count(point)) continue;

                const OL bottom_y = getpos(ys, (long)yi - 1), top_y = getpos(ys, (long)yi + 1);
                const OL right_x = getpos(xs, (long)xi + 1), left_x = getpos(xs, (long)xi - 1);

                std::vector<Cond> top_conditions, bottom_conditions, right_conditions, left_conditions;

                for (int dir : {HORIZONTAL, VERTICAL}) {
                    const bool is_horizontal_dir = (dir == HORIZONTAL);
                    const std::vector<long>& axis = is_horizontal_dir ? xs : ys;
                    const long index = is_horizontal_dir ? (long)xi : (long)yi;
                    const long pos = is_horizontal_dir ? x : y;
                    const long sub_pos = is_horizontal_dir ? y : x;
                    const OL sub_next = is_horizontal_dir ? top_y : right_x;
                    const OL sub_prev = is_horizontal_dir ? bottom_y : left_x;
                    std::vector<Cond>& next_conds = is_horizontal_dir ? top_conditions : right_conditions;
                    std::vector<Cond>& prev_conds = is_horizontal_dir ? bottom_conditions : left_conditions;
                    auto mk = [&](OL mainc, OL crossc) -> OArr {
                        if (!mainc || !crossc) return std::nullopt;
                        return is_horizontal_dir ? Arr{*mainc, *crossc, z} : Arr{*crossc, *mainc, z};
                    };
                    auto mkcond = [&](const std::set<std::string>& tgt, const std::set<std::string>& names) {
                        return logic::And(ctx, {logic::And(ctx, to_conds(tgt)),
                                                logic::Not(logic::Or(ctx, to_conds(names)))});
                    };
                    auto minus = [](std::set<std::string> s, const OS& a, const OS& b = std::nullopt) {
                        if (a) s.erase(*a);
                        if (b) s.erase(*b);
                        return s;
                    };

                    for (long si = index; si >= 0; --si) {
                        const long s_pos = axis[si];
                        const OArr s_point = mk(s_pos, sub_pos);
                        const long base_len = (pos - s_pos) + width_or_ext;
                        const bool done = base_len > max_tip_len;

                        const OS s_sub_next = ename(s_point, mk(s_pos, sub_next));
                        const OS s_sub_prev = ename(s_point, mk(s_pos, sub_prev));
                        std::set<std::string> sub_next_names, sub_prev_names, target_names;
                        if (s_sub_next) sub_next_names.insert(*s_sub_next);
                        if (s_sub_prev) sub_prev_names.insert(*s_sub_prev);

                        for (long li = si + 1; li < (long)axis.size(); ++li) {
                            const long l_pos = axis[li];
                            const OArr l_point = mk(l_pos, sub_pos);
                            const OS l_sub_next = ename(l_point, mk(l_pos, sub_next));
                            const OS l_sub_prev = ename(l_point, mk(l_pos, sub_prev));
                            if (l_sub_next) sub_next_names.insert(*l_sub_next);
                            if (l_sub_prev) sub_prev_names.insert(*l_sub_prev);

                            const OL prev_pos = getpos(axis, li - 1);
                            if (!prev_pos) continue;
                            const OArr prev_point = mk(*prev_pos, sub_pos);
                            const OS link = ename(prev_point, l_point);
                            if (!link) break;
                            target_names.insert(*link);

                            if (l_pos < pos) continue;
                            const long metal_length = (l_pos - s_pos) + width_or_ext;

                            if (routing_dir == BIDIRECTION) {
                                if (metal_length - 2 * layer_width > max_tip_len) {
                                    if (pos != s_pos && pos != l_pos) {
                                        next_conds.push_back(mkcond(target_names, minus(sub_next_names, s_sub_next, l_sub_next)));
                                        prev_conds.push_back(mkcond(target_names, minus(sub_prev_names, s_sub_prev, l_sub_prev)));
                                    } else {
                                        if (pos != s_pos) {
                                            next_conds.push_back(mkcond(target_names, minus(sub_next_names, s_sub_next)));
                                            prev_conds.push_back(mkcond(target_names, minus(sub_prev_names, s_sub_prev)));
                                        }
                                        if (pos != l_pos) {
                                            next_conds.push_back(mkcond(target_names, minus(sub_next_names, l_sub_next)));
                                            prev_conds.push_back(mkcond(target_names, minus(sub_prev_names, l_sub_prev)));
                                        }
                                    }
                                    break;
                                } else if (metal_length - layer_width > max_tip_len) {
                                    if (pos != s_pos) {
                                        next_conds.push_back(mkcond(target_names, minus(sub_next_names, s_sub_next)));
                                        prev_conds.push_back(mkcond(target_names, minus(sub_prev_names, s_sub_prev)));
                                    }
                                    if (pos != l_pos) {
                                        next_conds.push_back(mkcond(target_names, minus(sub_next_names, l_sub_next)));
                                        prev_conds.push_back(mkcond(target_names, minus(sub_prev_names, l_sub_prev)));
                                    }
                                } else if (metal_length > max_tip_len) {
                                    next_conds.push_back(mkcond(target_names, sub_next_names));
                                    prev_conds.push_back(mkcond(target_names, sub_prev_names));
                                }
                            } else {
                                if (metal_length > max_tip_len) {
                                    next_conds.push_back(mkcond(target_names, sub_next_names));
                                    prev_conds.push_back(mkcond(target_names, sub_prev_names));
                                    break;
                                }
                            }
                        }
                        if (done) break;
                    }
                }

                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("SIDE_T", x, y, z)), logic::Or(ctx, top_conditions)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("SIDE_B", x, y, z)), logic::Or(ctx, bottom_conditions)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("SIDE_R", x, y, z)), logic::Or(ctx, right_conditions)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("SIDE_L", x, y, z)), logic::Or(ctx, left_conditions)));
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::corner_helper(
    const Config& cfg,
    const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points,
    const RoutingGraph& g) {
    using Arr = std::array<long, 3>;
    using OArr = std::optional<Arr>;
    using OL = std::optional<long>;

    auto E = [&](const OArr& a, const OArr& b) -> Cond {
        if (!a || !b || !edge_in_graph(g,*a, *b)) return Cond::None();
        return Cond::Expr(bool_var(edge_var_name(*a, *b)));
    };
    auto with_edges = points_with_edges(g);

    std::map<std::string, int> layer_index = build_layer_index(cfg);

    std::vector<z3::expr> out;
    const int nz = (int)cfg.routing_layers.size();
    for (int z = 0; z < nz; ++z) {
        const std::string& layer = cfg.routing_layers[z];
        std::vector<int> lower_zs, upper_zs, same_height_zs;
        for (auto& l : cfg.lower_layers.at(layer)) if (layer_index.count(l)) lower_zs.push_back(layer_index[l]);
        for (auto& l : cfg.upper_layers.at(layer)) if (layer_index.count(l)) upper_zs.push_back(layer_index[l]);
        for (auto& l : cfg.same_height_layers.at(layer)) if (layer_index.count(l)) same_height_zs.push_back(layer_index[l]);
        const auto& xs = x_points[z];
        const auto& ys = y_points[z];

        for (size_t xi = 0; xi < xs.size(); ++xi) {
            const long x = xs[xi];
            for (size_t yi = 0; yi < ys.size(); ++yi) {
                const long y = ys[yi];
                const Arr point{x, y, z};
                if (!with_edges.count(point)) continue;

                const OL left_x = getpos(xs, (long)xi - 1), right_x = getpos(xs, (long)xi + 1);
                const OL bottom_y = getpos(ys, (long)yi - 1), top_y = getpos(ys, (long)yi + 1);
                auto pt = [&](OL px, OL py) -> OArr {
                    if (!px || !py) return std::nullopt;
                    return Arr{*px, *py, z};
                };
                const Cond right_e = E(point, pt(right_x, y)), left_e = E(point, pt(left_x, y));
                const Cond top_e = E(point, pt(x, top_y)), bottom_e = E(point, pt(x, bottom_y));
                std::vector<Cond> same_e, lower_e, upper_e;
                for (int shz : same_height_zs) { Cond c = E(point, Arr{x, y, shz}); if (c.kind != Cond::NONE) same_e.push_back(c); }
                for (int lz : lower_zs) { Cond c = E(point, Arr{x, y, lz}); if (c.kind != Cond::NONE) lower_e.push_back(c); }
                for (int uz : upper_zs) { Cond c = E(point, Arr{x, y, uz}); if (c.kind != Cond::NONE) upper_e.push_back(c); }

                auto orcat = [&](const Cond& a, const Cond& b) {
                    std::vector<Cond> v{a, b};
                    v.insert(v.end(), upper_e.begin(), upper_e.end());
                    v.insert(v.end(), lower_e.begin(), lower_e.end());
                    v.insert(v.end(), same_e.begin(), same_e.end());
                    return logic::Or(ctx, v);
                };
                auto corner = [&](const Cond& adjA, const Cond& adjB, const Cond& e1, const Cond& e2) {
                    Cond turn = logic::And(ctx, {orcat(adjA, adjB), logic::Not(logic::Or(ctx, {e1, e2}))});
                    Cond both = logic::And(ctx, {e1, e2}, /*must_match_num=*/2);
                    return logic::Or(ctx, {turn, both});
                };
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("CORNER_TL", x, y, z)),
                                               corner(right_e, bottom_e, left_e, top_e)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("CORNER_TR", x, y, z)),
                                               corner(left_e, bottom_e, right_e, top_e)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("CORNER_BL", x, y, z)),
                                               corner(right_e, top_e, left_e, bottom_e)));
                out.push_back(logic::eq_expr(ctx, bool_var(point_var_name("CORNER_BR", x, y, z)),
                                               corner(left_e, top_e, right_e, bottom_e)));
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::via_enclosure(
    const Config& cfg,
    const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points,
    const RoutingGraph& g) {
    using Arr = std::array<long, 3>;
    using OArr = std::optional<Arr>;
    using OL = std::optional<long>;

    auto E = [&](const Arr& a, const OArr& b) -> Cond {
        if (!b || !edge_in_graph(g,a, *b)) return Cond::None();
        return Cond::Expr(bool_var(edge_var_name(a, *b)));
    };
    std::map<std::string, int> layer_index = build_layer_index(cfg);

    auto emit = [](std::vector<z3::expr>& out, const Cond& c) {
        if (c.kind == Cond::EXPR) out.push_back(*c.e);
    };

    std::vector<z3::expr> out;
    const int nz = (int)cfg.routing_layers.size();
    for (int z = 0; z < nz; ++z) {
        const std::string& layer = cfg.routing_layers[z];
        const int routing_dir = cfg.routing_directions[z];
        std::vector<int> lower_zs, upper_zs;
        for (auto& l : cfg.lower_layers.at(layer)) if (layer_index.count(l)) lower_zs.push_back(layer_index[l]);
        for (auto& l : cfg.upper_layers.at(layer)) if (layer_index.count(l)) upper_zs.push_back(layer_index[l]);
        const auto& xs = x_points[z];
        const auto& ys = y_points[z];

        for (size_t xi = 0; xi < xs.size(); ++xi) {
            const long x = xs[xi];
            const OL left_x = getpos(xs, (long)xi - 1), right_x = getpos(xs, (long)xi + 1);
            for (size_t yi = 0; yi < ys.size(); ++yi) {
                const long y = ys[yi];
                const OL bottom_y = getpos(ys, (long)yi - 1), top_y = getpos(ys, (long)yi + 1);
                const Arr point{x, y, z};
                const bool is_node = g.nodes.count(point) > 0;
                auto venc = [&](const std::string& pre) -> Cond {
                    return is_node ? Cond::Expr(bool_var(point_var_name(pre, x, y, z))) : Cond::None();
                };
                auto pt = [&](OL px, OL py) -> OArr {
                    if (!px || !py) return std::nullopt;
                    return Arr{*px, *py, z};
                };
                const Cond hor_lower = venc("VIA_ENC_HOR_L"), hor_upper = venc("VIA_ENC_HOR_U");
                const Cond ver_lower = venc("VIA_ENC_VER_L"), ver_upper = venc("VIA_ENC_VER_U");

                const Cond hor_via_or = logic::Or(ctx, {hor_lower, hor_upper});
                const Cond ver_via_or = logic::Or(ctx, {ver_lower, ver_upper});
                const Cond lower_via_or = logic::Or(ctx, {hor_lower, ver_lower});
                const Cond upper_via_or = logic::Or(ctx, {hor_upper, ver_upper});

                if (routing_dir == VERTICAL) emit(out, logic::Not(hor_via_or));
                else if (routing_dir == HORIZONTAL) emit(out, logic::Not(ver_via_or));

                const Cond right_e = E(point, pt(right_x, y)), left_e = E(point, pt(left_x, y));
                const Cond top_e = E(point, pt(x, top_y)), bottom_e = E(point, pt(x, bottom_y));
                std::vector<Cond> lower_edges, upper_edges;
                for (int lz : lower_zs) { Cond c = E(point, Arr{x, y, lz}); if (c.kind != Cond::NONE) lower_edges.push_back(c); }
                for (int uz : upper_zs) { Cond c = E(point, Arr{x, y, uz}); if (c.kind != Cond::NONE) upper_edges.push_back(c); }

                emit(out, logic::Equal(ctx, logic::Or(ctx, lower_edges), lower_via_or));
                emit(out, logic::Equal(ctx, logic::Or(ctx, upper_edges), upper_via_or));
                emit(out, logic::Not(logic::And(ctx, {hor_via_or, ver_via_or})));
                emit(out, logic::Or(ctx, {logic::Not(logic::Or(ctx, {right_e, left_e})), logic::Not(ver_via_or)}));
                emit(out, logic::Or(ctx, {logic::Not(logic::Or(ctx, {top_e, bottom_e})), logic::Not(hor_via_or)}));
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::cross_layer_via_block(
    const Config& cfg,
    const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points,
    const RoutingGraph& g) {
    using Arr = std::array<long, 3>;
    std::vector<z3::expr> out;
    const int nz = (int)cfg.routing_layers.size();
    for (int curr_z = 0; curr_z < nz; ++curr_z) {
        const std::string& layer = cfg.routing_layers[curr_z];
        const long layer_width = cfg.width(layer);
        auto lv = cfg.lower_via.find(layer);
        if (lv == cfg.lower_via.end() || !lv->second.has_value()) continue;
        const long via_width = cfg.width(*lv->second);
        const auto& xs = x_points[curr_z];
        const auto& ys = y_points[curr_z];

        for (size_t vxi = 0; vxi < xs.size(); ++vxi) {
            const long cvx = xs[vxi];
            const double min_via_x = cvx - via_width / 2.0, max_via_x = cvx + via_width / 2.0;
            for (size_t vyi = 0; vyi < ys.size(); ++vyi) {
                const long cvy = ys[vyi];
                const double min_via_y = cvy - via_width / 2.0, max_via_y = cvy + via_width / 2.0;
                const Arr curr_point{cvx, cvy, curr_z};
                if (!g.nodes.count(curr_point)) continue;
                z3::expr venc_ver = bool_var(point_var_name("VIA_ENC_VER_L", cvx, cvy, curr_z));
                z3::expr venc_hor = bool_var(point_var_name("VIA_ENC_HOR_L", cvx, cvy, curr_z));

                for (size_t mxi = 0; mxi < xs.size(); ++mxi) {
                    const long cmx = xs[mxi];
                    const double min_metal_x = cmx - layer_width / 2.0, max_metal_x = cmx + layer_width / 2.0;
                    if (max_via_x < min_metal_x || max_metal_x < min_via_x) continue;
                    const bool x_match = (cmx <= cvx);

                    for (size_t myi = 0; myi < ys.size(); ++myi) {
                        const long cmy = ys[myi];
                        const double min_metal_y = cmy - layer_width / 2.0, max_metal_y = cmy + layer_width / 2.0;
                        const Arr metal_point{cmx, cmy, curr_z};
                        if (!g.nodes.count(metal_point)) continue;
                        if (max_via_y < min_metal_y || max_metal_y < min_via_y) continue;
                        const bool y_match = (cmy == cvy);

                        z3::expr base = bool_var(point_var_name("G", cmx, cmy, curr_z));
                        z3::expr mdir_hor = bool_var(point_var_name("METAL_HOR", cmx, cmy, curr_z));
                        z3::expr mdir_ver = bool_var(point_var_name("METAL_VER", cmx, cmy, curr_z));
                        z3::expr ver_const(ctx), hor_const(ctx);
                        if (x_match && y_match) {
                            ver_const = (!venc_ver) || (!mdir_hor);
                            hor_const = (!venc_hor) || (!mdir_ver);
                        } else if (x_match && !y_match) {
                            ver_const = (!venc_ver) || (!mdir_hor);
                            hor_const = (!venc_hor) || (!base);
                        } else if (!x_match && y_match) {
                            ver_const = (!venc_ver) || (!base);
                            hor_const = (!venc_hor) || (!mdir_ver);
                        } else {
                            ver_const = (!venc_ver) || (!base);
                            hor_const = (!venc_hor) || (!base);
                        }
                        out.push_back(ver_const);
                        out.push_back(hor_const);
                    }
                }
            }
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::minimum_area(
    const Config& cfg,
    const std::vector<std::vector<long>>& x_points,
    const std::vector<std::vector<long>>& y_points,
    const RoutingGraph& g) {
    using Arr = std::array<long, 3>;
    std::vector<z3::expr> out;
    const int nz = (int)cfg.routing_layers.size();
    for (int z = 0; z < nz; ++z) {
        const std::string& layer = cfg.routing_layers[z];
        const int routing_dir = cfg.routing_directions[z];
        const long layer_width = cfg.width(layer);
        const long extension = cfg.rules.at("extension").at(layer).get<long>();
        const double min_area = cfg.rules.at("min_area").at(layer).get<double>();
        const double min_length = min_area / layer_width;
        const auto& xs = x_points[z];
        const auto& ys = y_points[z];

        for (size_t xi = 0; xi < xs.size(); ++xi) {
            const long x = xs[xi];
            for (size_t yi = 0; yi < ys.size(); ++yi) {
                const long y = ys[yi];
                const Arr point{x, y, z};
                const bool is_node = g.nodes.count(point) > 0;
                z3::expr curr_h = is_node ? bool_var(point_var_name("TIP_L", x, y, z)) : ctx.bool_val(false);
                z3::expr curr_v = is_node ? bool_var(point_var_name("TIP_B", x, y, z)) : ctx.bool_val(false);

                std::vector<Cond> lvars;
                for (size_t i = xi; i < xs.size(); ++i) {
                    const long next_x = xs[i];
                    const long ext = (routing_dir == HORIZONTAL) ? extension : layer_width;
                    if ((double)((next_x - x) + ext) < min_length) {
                        const Arr np{next_x, y, z};
                        if (is_node && g.nodes.count(np)) {
                            z3::expr nv = bool_var(point_var_name("TIP_R", next_x, y, z));
                            lvars.push_back(Cond::Expr(!(curr_h && nv)));
                        }
                    } else break;
                }
                for (size_t i = yi; i < ys.size(); ++i) {
                    const long next_y = ys[i];
                    const long ext = (routing_dir == VERTICAL) ? extension : layer_width;
                    if ((double)((next_y - y) + ext) < min_length) {
                        const Arr np{x, next_y, z};
                        if (is_node && g.nodes.count(np)) {
                            z3::expr nv = bool_var(point_var_name("TIP_T", x, next_y, z));
                            lvars.push_back(Cond::Expr(!(curr_v && nv)));
                        }
                    } else break;
                }
                if (!lvars.empty()) {
                    Cond c = logic::Or(ctx, lvars);
                    if (c.kind == Cond::EXPR) out.push_back(*c.e);
                }
            }
        }
    }
    return out;
}

}
