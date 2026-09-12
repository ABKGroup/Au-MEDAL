// Edge-type-aware spacing constraints (S2S/S2T/T2T/C2C) and via spacing.
#include "smt.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include "variables.hpp"

namespace aumedal {
namespace {
using Arr = std::array<long, 3>;

long rl(const Config& c, const char* key, const std::string& layer) {
    return c.rules.at(key).at(layer).get<long>();
}
bool in_power(const Config& c, const std::string& layer) {
    return std::find(c.power_layer.begin(), c.power_layer.end(), layer) != c.power_layer.end();
}
double spacing_rule(const Config& c, const char* kind, const std::string& a, const std::string& b) {
    const auto& sp = c.rules.at("spacing");
    if (!sp.contains(kind)) return -1;
    const auto& k = sp.at(kind);
    if (!k.contains(a)) return -1;
    const auto& ka = k.at(a);
    if (!ka.contains(b)) return -1;
    return ka.at(b).get<double>();
}
long layer_enclosure(const Config& c, const std::optional<std::string>& via, const std::string& layer) {
    if (!via) return 0;
    const auto& enc = c.rules.at("enclosure");
    if (!enc.contains(*via)) return 0;
    const auto& ev = enc.at(*via);
    if (!ev.contains(layer)) return 0;
    double e = ev.at(layer).get<double>();
    return static_cast<long>(e + c.width(*via) / 2.0);
}
size_t bisect_right(const std::vector<long>& v, double key) {
    return std::upper_bound(v.begin(), v.end(), key,
                            [](double k, long e) { return k < (double)e; }) - v.begin();
}
size_t bisect_left(const std::vector<long>& v, double key) {
    return std::lower_bound(v.begin(), v.end(), key,
                            [](long e, double k) { return (double)e < k; }) - v.begin();
}

void spacing_worker(SmtModel& m, std::vector<z3::expr>& out, const Config& cfg,
                    const std::vector<Arr>& curr_points, const std::vector<long>& next_x_points,
                    const std::vector<long>& next_y_points, int curr_z, int next_z, double spacing,
                    bool is_horizontal, bool is_layer, bool need_curr_via_enc, bool need_next_via_enc,
                    const std::string& prefix1, const std::string& prefix2, bool eps_bounds,
                    const std::set<Arr>& vars_points) {
    if (spacing <= 0 || curr_points.empty() || next_x_points.empty() || next_y_points.empty()) return;

    std::string curr_layer, next_layer;
    int curr_dir, next_dir;
    long curr_low_enc = 0, curr_up_enc = 0, next_low_enc = 0, next_up_enc = 0;
    if (is_layer) {
        curr_layer = cfg.routing_layers[curr_z];
        next_layer = cfg.routing_layers[next_z];
        curr_dir = cfg.routing_directions[curr_z];
        next_dir = cfg.routing_directions[next_z];
        curr_low_enc = layer_enclosure(cfg, cfg.lower_via.at(curr_layer), curr_layer);
        curr_up_enc = layer_enclosure(cfg, cfg.upper_via.at(curr_layer), curr_layer);
        next_low_enc = layer_enclosure(cfg, cfg.lower_via.at(next_layer), next_layer);
        next_up_enc = layer_enclosure(cfg, cfg.upper_via.at(next_layer), next_layer);
    } else {
        curr_layer = cfg.vias[curr_z];
        next_layer = cfg.vias[next_z];
        curr_dir = BIDIRECTION;
        next_dir = BIDIRECTION;
    }

    auto max3 = [](long a, long b, long c) { return std::max(a, std::max(b, c)); };
    long max_curr_ex = max3(cfg.width(curr_layer), rl(cfg, "extension", curr_layer),
                            in_power(cfg, curr_layer) ? cfg.power_width(curr_layer) : 0);
    long max_next_ex = max3(cfg.width(next_layer), rl(cfg, "extension", next_layer),
                            in_power(cfg, next_layer) ? cfg.power_width(next_layer) : 0);
    if (is_layer) {
        max_curr_ex = max3(max_curr_ex, curr_low_enc, curr_up_enc);
        max_next_ex = max3(max_next_ex, next_low_enc, next_up_enc);
    }
    long max_width_curr = std::max(cfg.width(curr_layer), in_power(cfg, curr_layer) ? cfg.power_width(curr_layer) : 0L);
    long max_width_next = std::max(cfg.width(next_layer), in_power(cfg, next_layer) ? cfg.power_width(next_layer) : 0L);
    const double overlap_limit = (max_width_curr + max_width_next) / 2.0;
    const double eps = 1e-9;
    const long cell_height = cfg.cell_height;
    const long w_curr = cfg.width(curr_layer), w_next = cfg.width(next_layer);
    const double reach = spacing + max_curr_ex + max_next_ex;

    const std::string venc_base = is_horizontal ? "VIA_ENC_HOR" : "VIA_ENC_VER";
    const std::string venc_lower = venc_base + "_L", venc_upper = venc_base + "_U";

    auto node = [&](const Arr& p) { return vars_points.count(p) > 0; };

    for (const Arr& cp : curr_points) {
        const long curr_x = cp[0], curr_y = cp[1];
        if (!node(cp)) continue;
        z3::expr curr_var = m.bool_var(point_var_name(prefix1, curr_x, curr_y, curr_z));

        long curr_layer_ex;
        if ((curr_y % cell_height == 0) && in_power(cfg, curr_layer) && !is_horizontal)
            curr_layer_ex = cfg.power_width(curr_layer);
        else if ((curr_dir == HORIZONTAL && is_horizontal) || (curr_dir == VERTICAL && !is_horizontal))
            curr_layer_ex = rl(cfg, "extension", curr_layer);
        else
            curr_layer_ex = w_curr;

        size_t x_start, x_end, y_start, y_end;
        if (is_horizontal) {
            x_start = bisect_right(next_x_points, (double)curr_x);
            x_end = bisect_right(next_x_points, curr_x + reach);
            if (eps_bounds) {
                y_start = bisect_right(next_y_points, curr_y - overlap_limit + eps);
                y_end = bisect_left(next_y_points, curr_y + overlap_limit - eps);
            } else {
                y_start = bisect_left(next_y_points, curr_y - overlap_limit);
                y_end = bisect_right(next_y_points, curr_y + overlap_limit);
            }
        } else {
            if (eps_bounds) {
                x_start = bisect_right(next_x_points, curr_x - overlap_limit + eps);
                x_end = bisect_left(next_x_points, curr_x + overlap_limit - eps);
            } else {
                x_start = bisect_left(next_x_points, curr_x - overlap_limit);
                x_end = bisect_right(next_x_points, curr_x + overlap_limit);
            }
            y_start = bisect_right(next_y_points, (double)curr_y);
            y_end = bisect_right(next_y_points, curr_y + reach);
        }

        std::optional<z3::expr> curr_low_venc, curr_up_venc;
        if (is_layer) {
            if (node(cp)) {
                curr_low_venc = m.bool_var(point_var_name(venc_lower, curr_x, curr_y, curr_z));
                curr_up_venc = m.bool_var(point_var_name(venc_upper, curr_x, curr_y, curr_z));
            }
        }

        for (size_t xi = x_start; xi < x_end; ++xi) {
            const long next_x = next_x_points[xi];
            if (is_horizontal && (double)(next_x - curr_x) >= reach) break;
            for (size_t yi = y_start; yi < y_end; ++yi) {
                const long next_y = next_y_points[yi];
                if (!is_horizontal && (double)(next_y - curr_y) >= reach) break;
                const Arr np{next_x, next_y, next_z};
                if (!node(np)) continue;
                z3::expr next_var = m.bool_var(point_var_name(prefix2, next_x, next_y, next_z));

                long next_layer_ex;
                if ((next_y % cell_height == 0) && in_power(cfg, next_layer) && !is_horizontal)
                    next_layer_ex = cfg.power_width(next_layer);
                else if ((next_dir == HORIZONTAL && is_horizontal) || (next_dir == VERTICAL && !is_horizontal))
                    next_layer_ex = rl(cfg, "extension", next_layer);
                else
                    next_layer_ex = w_next;

                double overlapped_metal =
                    std::abs((double)(is_horizontal ? (next_y - curr_y) : (next_x - curr_x))) - w_curr / 2.0 - w_next / 2.0;
                if (overlapped_metal >= 0) continue;

                std::optional<z3::expr> next_low_venc, next_up_venc;
                if (is_layer) {
                    next_low_venc = m.bool_var(point_var_name(venc_lower, next_x, next_y, next_z));
                    next_up_venc = m.bool_var(point_var_name(venc_upper, next_x, next_y, next_z));
                }

                const long unit_spacing = is_horizontal ? (next_x - curr_x) : (next_y - curr_y);
                const double curr_ex = curr_layer_ex / 2.0;
                const double next_ex = next_layer_ex / 2.0;

                if ((unit_spacing - curr_ex - next_ex) < spacing)
                    out.push_back((!(curr_var && next_var)));

                if (is_layer) {
                    const double curr_low_via_ex = std::max((double)curr_low_enc, curr_ex);
                    const double curr_up_via_ex = std::max((double)curr_up_enc, curr_ex);
                    const double next_low_via_ex = std::max((double)next_low_enc, next_ex);
                    const double next_up_via_ex = std::max((double)next_up_enc, next_ex);

                    if (need_next_via_enc) {
                        if ((unit_spacing - curr_ex - next_low_via_ex) < spacing && next_low_venc)
                            out.push_back((!(curr_var && next_var && *next_low_venc)));
                        if ((unit_spacing - curr_ex - next_up_via_ex) < spacing && next_up_venc)
                            out.push_back((!(curr_var && next_var && *next_up_venc)));
                    }
                    if (need_curr_via_enc) {
                        if ((unit_spacing - curr_low_via_ex - next_ex) < spacing && curr_low_venc)
                            out.push_back((!(curr_var && next_var && *curr_low_venc)));
                        if ((unit_spacing - curr_up_via_ex - next_ex) < spacing && curr_up_venc)
                            out.push_back((!(curr_var && next_var && *curr_up_venc)));
                    }
                    if (need_curr_via_enc && need_next_via_enc) {
                        const double spacing_low_low = unit_spacing - curr_low_via_ex - next_low_via_ex;
                        const double spacing_low_up = unit_spacing - curr_low_via_ex - next_up_via_ex;
                        const double spacing_up_low = unit_spacing - curr_up_via_ex - next_low_via_ex;
                        const double spacing_up_up = unit_spacing - curr_up_via_ex - next_up_via_ex;
                        if (spacing_low_low < spacing && next_low_venc && curr_low_venc)
                            out.push_back((!(curr_var && next_var && *curr_low_venc && *next_low_venc)));
                        if (spacing_low_up < spacing && next_up_venc && curr_low_venc)
                            out.push_back((!(curr_var && next_var && *curr_low_venc && *next_up_venc)));
                        if (spacing_up_low < spacing && curr_up_venc && next_low_venc)
                            out.push_back((!(curr_var && next_var && *curr_up_venc && *next_low_venc)));
                        if (spacing_up_up < spacing && curr_up_venc && next_up_venc)
                            out.push_back((!(curr_var && next_var && *curr_up_venc && *next_up_venc)));
                        if (spacing <= std::min(std::min(spacing_low_low, spacing_low_up),
                                                std::min(spacing_up_low, spacing_up_up))) break;
                    }
                }
            }
        }
    }
}

std::map<int, std::vector<Arr>> nodes_by_z(const RoutingGraph& g) {
    std::map<int, std::vector<Arr>> m;
    for (const auto& n : g.nodes) m[(int)n[2]].push_back(n);
    return m;
}
void coords_by_z(const RoutingGraph& g, int nz, std::map<int, std::vector<long>>& xs,
                 std::map<int, std::vector<long>>& ys) {
    std::map<int, std::set<long>> sx, sy;
    for (int z = 0; z < nz; ++z) { sx[z]; sy[z]; }
    for (const auto& n : g.nodes) { sx[(int)n[2]].insert(n[0]); sy[(int)n[2]].insert(n[1]); }
    for (auto& kv : sx) xs[kv.first] = std::vector<long>(kv.second.begin(), kv.second.end());
    for (auto& kv : sy) ys[kv.first] = std::vector<long>(kv.second.begin(), kv.second.end());
}

void corner_kernel(SmtModel& m, std::vector<z3::expr>& out, const Config& cfg,
                   const Arr& curr_point, const std::vector<long>& next_x_points,
                   const std::vector<long>& next_y_points, int curr_z, int next_z, double spacing,
                   bool is_upward, bool is_layer, const std::string& prefix1, const std::string& prefix2,
                   const std::set<Arr>& vars_points) {
    auto dist_sq = [](double dx, double dy) { return dx * dx + dy * dy; };
    if (spacing < 0) return;
    const long curr_x = curr_point[0], curr_y = curr_point[1];
    auto node = [&](const Arr& p) { return vars_points.count(p) > 0; };
    if (!node(curr_point)) return;
    z3::expr curr_var = m.bool_var(point_var_name(prefix1, curr_x, curr_y, curr_z));

    const std::string curr_layer = is_layer ? cfg.routing_layers[curr_z] : cfg.vias[curr_z];
    const std::string next_layer = is_layer ? cfg.routing_layers[next_z] : cfg.vias[next_z];
    const long curr_low_enc = is_layer ? layer_enclosure(cfg, cfg.lower_via.at(curr_layer), curr_layer) : 0;
    const long curr_up_enc = is_layer ? layer_enclosure(cfg, cfg.upper_via.at(curr_layer), curr_layer) : 0;
    const long next_low_enc = is_layer ? layer_enclosure(cfg, cfg.lower_via.at(next_layer), next_layer) : 0;
    const long next_up_enc = is_layer ? layer_enclosure(cfg, cfg.upper_via.at(next_layer), next_layer) : 0;

    const double curr_x_ex = std::max(cfg.width(curr_layer), rl(cfg, "extension", curr_layer));
    auto max4 = [](double a, double b, double c, double d) { return std::max(std::max(a, b), std::max(c, d)); };
    const double max_curr_prune = max4(curr_x_ex / 2, curr_low_enc, curr_up_enc,
                                       in_power(cfg, curr_layer) ? cfg.power_width(curr_layer) / 2.0 : 0.0);
    const double max_next_prune = max4(std::max(cfg.width(next_layer), rl(cfg, "extension", next_layer)) / 2.0,
                                       next_low_enc, next_up_enc,
                                       in_power(cfg, next_layer) ? cfg.power_width(next_layer) / 2.0 : 0.0);
    const double spacing_sq = spacing * spacing;
    const long cell_height = cfg.cell_height;
    const int curr_rdir = cfg.routing_directions[curr_z];
    const int next_rdir = cfg.routing_directions[next_z];

    for (int corder = 0; corder < 2; ++corder) {
        const int curr_dir = (corder == 0) ? HORIZONTAL : VERTICAL;
        if (!is_layer && corder == 1) break;
        if (is_layer && curr_rdir != BIDIRECTION && curr_rdir != curr_dir) continue;
        const std::string venc_c = (curr_dir == HORIZONTAL) ? "VIA_ENC_HOR" : "VIA_ENC_VER";
        z3::expr curr_low_venc = is_layer ? m.bool_var(point_var_name(venc_c + "_L", curr_x, curr_y, curr_z)) : m.ctx.bool_val(false);
        z3::expr curr_up_venc = is_layer ? m.bool_var(point_var_name(venc_c + "_U", curr_x, curr_y, curr_z)) : m.ctx.bool_val(false);

        for (int norder = 0; norder < 2; ++norder) {
            const int next_dir = (norder == 0) ? HORIZONTAL : VERTICAL;
            if (!is_layer && norder == 1) break;
            if (is_layer && next_rdir != BIDIRECTION && next_rdir != next_dir) continue;
            const std::string venc_n = (next_dir == HORIZONTAL) ? "VIA_ENC_HOR" : "VIA_ENC_VER";

            size_t x_start = bisect_right(next_x_points, (double)curr_x);
            for (size_t xi = x_start; xi < next_x_points.size(); ++xi) {
                const long next_x = next_x_points[xi];
                const long x_unit = next_x - curr_x;
                if (x_unit - max_curr_prune - max_next_prune >= spacing) break;
                const double next_x_ex = std::max(cfg.width(next_layer), rl(cfg, "extension", next_layer));

                auto scan_next_y = [&](long next_y) -> bool {
                    const long y_unit = is_upward ? (next_y - curr_y) : (curr_y - next_y);
                    if (dist_sq(x_unit - max_curr_prune - max_next_prune, y_unit - max_curr_prune - max_next_prune) >= spacing_sq)
                        return false;
                    const double curr_y_ex = (curr_y % cell_height == 0 && in_power(cfg, curr_layer))
                                                 ? (double)cfg.power_width(curr_layer) : curr_x_ex;
                    const double next_y_ex = (next_y % cell_height == 0 && in_power(cfg, next_layer))
                                                 ? (double)cfg.power_width(next_layer) : next_x_ex;
                    const Arr np{next_x, next_y, next_z};
                    if (!node(np)) return true;
                    z3::expr next_var = m.bool_var(point_var_name(prefix2, next_x, next_y, next_z));
                    auto base = [&]() { return !(curr_var && next_var); };
                    auto emit_cond = [&](const std::optional<z3::expr>& cond) {
                        if (!cond) out.push_back(base());
                        else out.push_back(((!*cond) || base()));
                    };

                    if (dist_sq(x_unit - curr_x_ex / 2 - next_x_ex / 2, y_unit - curr_y_ex / 2 - next_y_ex / 2) < spacing_sq)
                        out.push_back(base());

                    if (!is_layer) return true;

                    z3::expr next_low_venc = m.bool_var(point_var_name(venc_n + "_L", next_x, next_y, next_z));
                    z3::expr next_up_venc = m.bool_var(point_var_name(venc_n + "_U", next_x, next_y, next_z));

                    struct Tier { long e1, e2; z3::expr cv, nv; };
                    Tier same[2] = {{curr_low_enc, next_low_enc, curr_low_venc, next_low_venc},
                                    {curr_up_enc, next_up_enc, curr_up_venc, next_up_venc}};
                    for (auto& t : same) {
                        for (int axis : {HORIZONTAL, VERTICAL}) {
                            const bool dxm = (axis == HORIZONTAL), dym = (axis == VERTICAL);
                            if (curr_dir == axis) {
                                double x_ex = dxm ? std::max((double)t.e1, curr_x_ex / 2) : curr_x_ex / 2;
                                double y_ex = dym ? std::max((double)t.e1, curr_y_ex / 2) : curr_y_ex / 2;
                                if (dist_sq(x_unit - next_x_ex / 2 - x_ex, y_unit - next_y_ex / 2 - y_ex) < spacing_sq)
                                    emit_cond(t.cv);
                            }
                            if (next_dir == axis) {
                                double x_ex = dxm ? std::max((double)t.e2, next_x_ex / 2) : next_x_ex / 2;
                                double y_ex = dym ? std::max((double)t.e2, next_y_ex / 2) : next_y_ex / 2;
                                if (dist_sq(x_unit - curr_x_ex / 2 - x_ex, y_unit - curr_y_ex / 2 - y_ex) < spacing_sq)
                                    emit_cond(t.nv);
                            }
                            for (int axis2 : {HORIZONTAL, VERTICAL}) {
                                const bool dxm2 = (axis2 == HORIZONTAL), dym2 = (axis2 == VERTICAL);
                                if (curr_dir == axis && next_dir == axis2) {
                                    double x_ex1 = dxm ? std::max((double)t.e1, curr_x_ex / 2) : curr_x_ex / 2;
                                    double y_ex1 = dym ? std::max((double)t.e1, curr_y_ex / 2) : curr_y_ex / 2;
                                    double x_ex2 = dxm2 ? std::max((double)t.e2, next_x_ex / 2) : next_x_ex / 2;
                                    double y_ex2 = dym2 ? std::max((double)t.e2, next_y_ex / 2) : next_y_ex / 2;
                                    if (dist_sq(x_unit - x_ex1 - x_ex2, y_unit - y_ex1 - y_ex2) < spacing_sq)
                                        out.push_back(((!(t.cv && t.nv)) || base()));
                                }
                            }
                        }
                    }
                    Tier cross[2] = {{curr_low_enc, next_up_enc, curr_low_venc, next_up_venc},
                                     {curr_up_enc, next_low_enc, curr_up_venc, next_low_venc}};
                    for (auto& t : cross) {
                        for (int axis1 : {HORIZONTAL, VERTICAL}) {
                            const bool dxm1 = (axis1 == HORIZONTAL), dym1 = (axis1 == VERTICAL);
                            for (int axis2 : {HORIZONTAL, VERTICAL}) {
                                const bool dxm2 = (axis2 == HORIZONTAL), dym2 = (axis2 == VERTICAL);
                                if (curr_dir == axis1 && next_dir == axis2) {
                                    double x_ex1 = dxm1 ? std::max((double)t.e1, curr_x_ex / 2) : curr_x_ex / 2;
                                    double y_ex1 = dym1 ? std::max((double)t.e1, curr_y_ex / 2) : curr_y_ex / 2;
                                    double x_ex2 = dxm2 ? std::max((double)t.e2, next_x_ex / 2) : next_x_ex / 2;
                                    double y_ex2 = dym2 ? std::max((double)t.e2, next_y_ex / 2) : next_y_ex / 2;
                                    if (dist_sq(x_unit - x_ex1 - x_ex2, y_unit - y_ex1 - y_ex2) < spacing_sq)
                                        out.push_back(((!(t.cv && t.nv)) || base()));
                                }
                            }
                        }
                    }
                    return true;
                };

                if (is_upward) {
                    for (size_t yi = bisect_right(next_y_points, (double)curr_y); yi < next_y_points.size(); ++yi)
                        if (!scan_next_y(next_y_points[yi])) break;
                } else {
                    long start = (long)bisect_left(next_y_points, (double)curr_y) - 1;
                    for (long yi = start; yi >= 0; --yi)
                        if (!scan_next_y(next_y_points[yi])) break;
                }
            }
        }
    }
}

std::set<Arr> compute_via_points(const Config& cfg, const std::vector<std::vector<long>>& x_points,
                                 const std::vector<std::vector<long>>& y_points, const RoutingGraph& g) {
    auto edge_exists = [&](const Arr& a, const Arr& b) {
        auto e = (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
        return g.edges.count(e) > 0;
    };
    std::map<std::string, int> layer_index;
    for (int i = 0; i < (int)cfg.routing_layers.size(); ++i) layer_index[cfg.routing_layers[i]] = i;
    std::map<std::string, int> via_index;
    for (int i = 0; i < (int)cfg.vias.size(); ++i) via_index[cfg.vias[i]] = i;
    std::map<Arr, int> via_count;
    std::set<Arr> result;
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
        const int via_idx = vit->second;
        for (long x : x_points[z]) {
            for (long y : y_points[z]) {
                const Arr point{x, y, z};
                const Arr via_key{x, y, via_idx};
                for (int uz : upper_zs)
                    if (edge_exists(point, Arr{x, y, uz})) via_count[via_key]++;
                if (z == max_same) result.insert(via_key);
            }
        }
    }
    return result;
}

std::vector<z3::expr> linear_spacing(SmtModel& m, const Config& cfg, const RoutingGraph& g,
                                        const char* kind, const std::string& h_curr, const std::string& h_next,
                                        const std::string& v_curr, const std::string& v_next,
                                        bool need_curr_h, bool need_next_h, bool need_curr_v, bool need_next_v) {
    const int nz = (int)cfg.routing_layers.size();
    auto nbz = nodes_by_z(g);
    std::map<int, std::vector<long>> xbz, ybz;
    coords_by_z(g, nz, xbz, ybz);
    std::vector<z3::expr> out;
    for (int cz = 0; cz < nz; ++cz) {
        for (int nzi = 0; nzi < nz; ++nzi) {
            double sp = spacing_rule(cfg, kind, cfg.routing_layers[cz], cfg.routing_layers[nzi]);
            if (sp < 0) continue;
            spacing_worker(m, out, cfg, nbz[cz], xbz[nzi], ybz[nzi], cz, nzi, sp,
                           /*is_horizontal=*/true, /*is_layer=*/true, need_curr_h, need_next_h,
                           h_curr, h_next, /*eps_bounds=*/true, g.nodes);
            spacing_worker(m, out, cfg, nbz[cz], xbz[nzi], ybz[nzi], cz, nzi, sp,
                           /*is_horizontal=*/false, /*is_layer=*/true, need_curr_v, need_next_v,
                           v_curr, v_next, /*eps_bounds=*/true, g.nodes);
        }
    }
    return out;
}
}

std::vector<z3::expr> SmtModel::add_side_to_side_spacing(const Config& cfg,
                                                            const std::vector<std::vector<long>>&,
                                                            const std::vector<std::vector<long>>&,
                                                            const RoutingGraph& g) {
    return linear_spacing(*this, cfg, g, "S2S", "SIDE_R", "SIDE_L", "SIDE_T", "SIDE_B",
                          true, true, true, true);
}

std::vector<z3::expr> SmtModel::add_tip_to_tip_spacing(const Config& cfg,
                                                          const std::vector<std::vector<long>>&,
                                                          const std::vector<std::vector<long>>&,
                                                          const RoutingGraph& g) {
    return linear_spacing(*this, cfg, g, "T2T", "TIP_R", "TIP_L", "TIP_T", "TIP_B",
                          true, true, true, true);
}

std::vector<z3::expr> SmtModel::add_side_to_tip_spacing(const Config& cfg,
                                                           const std::vector<std::vector<long>>&,
                                                           const std::vector<std::vector<long>>&,
                                                           const RoutingGraph& g) {
    const int nz = (int)cfg.routing_layers.size();
    auto nbz = nodes_by_z(g);
    std::map<int, std::vector<long>> xbz, ybz;
    coords_by_z(g, nz, xbz, ybz);
    std::vector<z3::expr> out;
    for (int cz = 0; cz < nz; ++cz) {
        for (int nzi = 0; nzi < nz; ++nzi) {
            double sp = spacing_rule(cfg, "S2T", cfg.routing_layers[cz], cfg.routing_layers[nzi]);
            if (sp < 0) continue;
            spacing_worker(*this, out, cfg, nbz[cz], xbz[nzi], ybz[nzi], cz, nzi, sp,
                           /*is_horizontal=*/true, /*is_layer=*/true, /*need_curr_via_enc=*/false,
                           /*need_next_via_enc=*/true, "SIDE_R", "TIP_L", /*eps_bounds=*/true, g.nodes);
            spacing_worker(*this, out, cfg, nbz[cz], xbz[nzi], ybz[nzi], cz, nzi, sp,
                           /*is_horizontal=*/true, /*is_layer=*/true, /*need_curr_via_enc=*/true,
                           /*need_next_via_enc=*/false, "TIP_R", "SIDE_L", /*eps_bounds=*/true, g.nodes);
            spacing_worker(*this, out, cfg, nbz[cz], xbz[nzi], ybz[nzi], cz, nzi, sp,
                           /*is_horizontal=*/false, /*is_layer=*/true, /*need_curr_via_enc=*/false,
                           /*need_next_via_enc=*/true, "SIDE_T", "TIP_B", /*eps_bounds=*/true, g.nodes);
            spacing_worker(*this, out, cfg, nbz[cz], xbz[nzi], ybz[nzi], cz, nzi, sp,
                           /*is_horizontal=*/false, /*is_layer=*/true, /*need_curr_via_enc=*/true,
                           /*need_next_via_enc=*/false, "TIP_T", "SIDE_B", /*eps_bounds=*/true, g.nodes);
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_corner_spacing(const Config& cfg,
                                                      const std::vector<std::vector<long>>&,
                                                      const std::vector<std::vector<long>>&,
                                                      const RoutingGraph& g) {
    const int nz = (int)cfg.routing_layers.size();
    auto nbz = nodes_by_z(g);
    std::map<int, std::vector<long>> xbz, ybz;
    coords_by_z(g, nz, xbz, ybz);
    std::vector<z3::expr> out;
    for (int cz = 0; cz < nz; ++cz) {
        for (int nzi = 0; nzi < nz; ++nzi) {
            double sp = spacing_rule(cfg, "C2C", cfg.routing_layers[cz], cfg.routing_layers[nzi]);
            if (sp < 0) continue;
            for (const Arr& cp : nbz[cz])
                corner_kernel(*this, out, cfg, cp, xbz[nzi], ybz[nzi], cz, nzi, sp, true, true, "CORNER_TR", "CORNER_BL", g.nodes);
            for (const Arr& cp : nbz[cz])
                corner_kernel(*this, out, cfg, cp, xbz[nzi], ybz[nzi], cz, nzi, sp, false, true, "CORNER_BR", "CORNER_TL", g.nodes);
        }
    }
    return out;
}

std::vector<z3::expr> SmtModel::add_via_spacing(const Config& cfg,
                                                   const std::vector<std::vector<long>>& x_points,
                                                   const std::vector<std::vector<long>>& y_points,
                                                   const RoutingGraph& g) {
    const std::set<Arr> via_pts = compute_via_points(cfg, x_points, y_points, g);
    const int nlayers = (int)cfg.routing_layers.size();
    std::map<std::string, int> layer_index;
    for (int i = 0; i < nlayers; ++i) layer_index[cfg.routing_layers[i]] = i;
    std::vector<std::set<long>> xset(nlayers), yset(nlayers);
    for (int z = 0; z < nlayers; ++z) {
        xset[z] = std::set<long>(x_points[z].begin(), x_points[z].end());
        yset[z] = std::set<long>(y_points[z].begin(), y_points[z].end());
    }
    const int nvias = (int)cfg.vias.size();
    std::vector<std::vector<long>> via_x(nvias), via_y(nvias);
    for (int vz = 0; vz < nvias; ++vz) {
        const std::string& via = cfg.vias[vz];
        std::set<long> vx, vy;
        if (cfg.lower_layers.count(via) && cfg.upper_layers.count(via)) {
            for (const auto& lo : cfg.lower_layers.at(via)) {
                if (!layer_index.count(lo)) continue;
                int li = layer_index[lo];
                for (const auto& up : cfg.upper_layers.at(via)) {
                    if (!layer_index.count(up)) continue;
                    int ui = layer_index[up];
                    for (long x : xset[li]) if (xset[ui].count(x)) vx.insert(x);
                    for (long y : yset[li]) if (yset[ui].count(y)) vy.insert(y);
                }
            }
        }
        via_x[vz] = std::vector<long>(vx.begin(), vx.end());
        via_y[vz] = std::vector<long>(vy.begin(), vy.end());
    }

    std::vector<z3::expr> out;
    for (int cz = 0; cz < nvias; ++cz) {
        for (int nz = 0; nz < nvias; ++nz) {
            double t2t = spacing_rule(cfg, "T2T", cfg.vias[cz], cfg.vias[nz]);
            double c2c = spacing_rule(cfg, "C2C", cfg.vias[cz], cfg.vias[nz]);
            if (t2t < 0 && c2c < 0) continue;
            std::vector<Arr> curr;
            for (long x : via_x[cz]) for (long y : via_y[cz]) curr.push_back(Arr{x, y, cz});
            spacing_worker(*this, out, cfg, curr, via_x[nz], via_y[nz], cz, nz, t2t,
                           /*is_horizontal=*/true, /*is_layer=*/false, /*need_curr_via_enc=*/false,
                           /*need_next_via_enc=*/false, "VIA", "VIA", /*eps_bounds=*/false, via_pts);
            spacing_worker(*this, out, cfg, curr, via_x[nz], via_y[nz], cz, nz, t2t,
                           /*is_horizontal=*/false, /*is_layer=*/false, /*need_curr_via_enc=*/false,
                           /*need_next_via_enc=*/false, "VIA", "VIA", /*eps_bounds=*/false, via_pts);
            for (const Arr& cp : curr)
                corner_kernel(*this, out, cfg, cp, via_x[nz], via_y[nz], cz, nz, c2c, true, false, "VIA", "VIA", via_pts);
            for (const Arr& cp : curr)
                corner_kernel(*this, out, cfg, cp, via_x[nz], via_y[nz], cz, nz, c2c, false, false, "VIA", "VIA", via_pts);
        }
    }
    return out;
}

}
