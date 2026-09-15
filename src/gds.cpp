// Emits the routed and device geometry as rectangles and writes the GDSII.
#include "gds.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

namespace aumedal {

// get_square/merge_collinear declared in gds.hpp (shared with solve.cpp's
// post-solve DRC conflict checks, so they reconstruct exactly the
// geometry this file emits -- no independent re-derivation to drift out
// of sync). Rect is also declared there.
long ext_of(const Config& cfg, const std::string& layer) {
    return cfg.rules.at("extension").at(layer).get<long>();
}

bool over_active(const PreLayout& pre, const Rect& box) {
    for (const auto& a : pre.active)
        if (box[0] <= a[2] && box[2] >= a[0] && box[1] <= a[3] && box[3] >= a[1]) return true;
    return false;
}

Rect get_square(const Config& cfg, long x0, long y0, long x1, long y1,
                const std::string& layer, const PreLayout& pre) {
    const int zi = cfg.routing_layer_index(layer);
    const int rdir = (zi >= 0) ? cfg.routing_directions[zi] : BIDIRECTION;
    const long ch = cfg.cell_height;
    auto exts = [&](long y_value, long& x_ex, long& y_ex) {
        const bool pw = (y_value % ch == 0) && cfg.is_power_layer(layer);
        if (pw) { x_ex = cfg.width(layer); y_ex = cfg.power_width(layer); }
        else {
            x_ex = (rdir == HORIZONTAL) ? ext_of(cfg, layer) : cfg.width(layer);
            y_ex = (rdir == VERTICAL) ? ext_of(cfg, layer) : cfg.width(layer);
        }
    };
    long x_ex, y_ex;
    auto clear_gate_column = [&](long& x_extent) {
        if (layer != cfg.active_contact_layer) return;
        const long gap = cfg.spacing_s2s(layer, "Gate").value_or(0);
        const long room = 2 * ((long)cfg.x_unit - cfg.width("Gate") / 2 - gap) - 10;
        if (room > 0 && room < x_extent) x_extent = std::max(room, (long)cfg.width("Cont"));
    };
    if (x0 == x1 && y0 == y1) {
        exts(y0, x_ex, y_ex);
        clear_gate_column(x_ex);
        return {(double)(x0 - x_ex / 2), (double)(y0 - y_ex / 2),
                (double)(x0 + x_ex / 2), (double)(y0 + y_ex / 2)};
    }
    if (x0 == x1) {
        exts(y0, x_ex, y_ex);
        clear_gate_column(x_ex);
        return {(double)(x0 - x_ex / 2), (double)(std::min(y0, y1) - y_ex / 2),
                (double)(x0 + x_ex / 2), (double)(std::max(y0, y1) + y_ex / 2)};
    }
    exts(y0, x_ex, y_ex);
    long ymin = y0 - y_ex / 2, ymax = y0 + y_ex / 2;
    long xmin = std::min(x0, x1) - x_ex / 2, xmax = std::max(x0, x1) + x_ex / 2;
    if (layer == cfg.active_contact_layer) {
        const std::string& pl = cfg.power_layer.back();
        const long offset = cfg.pitch.at("fin") - (cfg.width(layer) / 2 + cfg.width("M1") / 2);
        const long s2s = cfg.spacing_s2s(pl, pl).value_or(0);
        Rect box{(double)xmin, (double)ymin, (double)xmax, (double)ymax};
        if (y0 == cfg.power_width(pl) + s2s && over_active(pre, box)) ymax += offset;
        if (y0 == ch - cfg.power_width(pl) - s2s && over_active(pre, box)) ymin -= offset;
    }
    return {(double)xmin, (double)ymin, (double)xmax, (double)ymax};
}

std::vector<std::pair<std::array<long, 2>, std::array<long, 2>>>
merge_collinear(const std::vector<std::pair<std::array<long, 2>, std::array<long, 2>>>& edges) {
    using P = std::array<long, 2>;
    auto run = [](const std::vector<std::pair<P, P>>& es) {
        std::map<P, int> id;
        auto get = [&](const P& p) { auto it = id.find(p); if (it != id.end()) return it->second; int n = (int)id.size(); id[p] = n; return n; };
        std::vector<int> parent;
        std::function<int(int)> find = [&](int a) { while (parent[a] != a) { parent[a] = parent[parent[a]]; a = parent[a]; } return a; };
        std::vector<std::pair<int, int>> pe;
        std::vector<P> pts;
        for (const auto& e : es) {
            int u = get(e.first), v = get(e.second);
            while ((int)parent.size() < (int)id.size()) { parent.push_back((int)parent.size()); pts.push_back({}); }
            pts[u] = e.first; pts[v] = e.second;
            pe.push_back({u, v});
        }
        for (auto& e : pe) parent[find(e.first)] = find(e.second);
        std::map<int, std::pair<P, P>> comp;
        for (const auto& kv : id) {
            int r = find(kv.second); const P& p = pts[kv.second];
            auto it = comp.find(r);
            if (it == comp.end()) comp[r] = {p, p};
            else { if (p < it->second.first) it->second.first = p; if (it->second.second < p) it->second.second = p; }
        }
        std::vector<std::pair<P, P>> out;
        for (auto& kv : comp) out.push_back(kv.second);
        return out;
    };
    std::vector<std::pair<P, P>> hor, ver;
    for (const auto& e : edges) {
        if (e.first[1] == e.second[1]) hor.push_back(e);
        else if (e.first[0] == e.second[0]) ver.push_back(e);
    }
    std::vector<std::pair<P, P>> out = run(hor);
    auto v = run(ver);
    out.insert(out.end(), v.begin(), v.end());
    return out;
}

namespace {

struct GdsLabel { std::string text; double x, y; int layer; int texttype; };

int label_texttype(const Config& cfg, const std::string& layer) {
    static const nlohmann::json empty = nlohmann::json::object();
    const auto& m = cfg.option.value("label_texttype", empty);
    if (m.contains(layer)) return m.at(layer).get<int>();
    return 25;
}

bool has_layer(const Config& cfg, const std::string& name) {
    return cfg.specs.at("layer_map").contains(name);
}
int gds_layer(const Config& cfg, const std::string& layer) {
    return cfg.specs.at("layer_map").at(layer).get<int>();
}
long off(const Config& cfg, const std::string& a, const std::string& b) {
    return cfg.rules.at("offset").at(a).at(b).get<long>();
}

bool bulk_planar(const Config& cfg) {
    return cfg.opt_bool("bulk_planar");
}

double active_y_offset(const Config& cfg) {
    return bulk_planar(cfg) ? cfg.opt_long("active_y_offset")
                            : cfg.pitch.at("M1") - cfg.width("M1") / 2.0;
}

double active_gate_extension(const Config& cfg) {
    return bulk_planar(cfg) ? cfg.opt_long("active_gate_extension")
                            : off(cfg, "Gate", "Active");
}

inline void clamp_rect(Rect& r, double lo, double hi) { r[1] = std::max(r[1], lo); r[3] = std::min(r[3], hi); }

const Net* find_net_by_name(const std::vector<Net>& nets, const std::string& name) {
    for (const Net& n : nets) if (n.name == name) return &n;
    for (const Net& n : nets) if (n.name.find(name) != std::string::npos) return &n;
    return nullptr;
}

std::pair<double, double> p_fin_extent(long cell_height, double y_off, double fin_pitch, long fin_count) {
    double max_y = cell_height - y_off;
    return {max_y - fin_pitch * fin_count, max_y};
}
std::pair<double, double> n_fin_extent(double y_off, double fin_pitch, long fin_count) {
    return {y_off, y_off + fin_pitch * fin_count};
}

std::vector<Rect> union_rects(const std::vector<Rect>& in) {
    if (in.size() <= 1) return in;
    std::vector<double> xs;
    xs.reserve(in.size() * 2);
    for (const auto& r : in) { xs.push_back(r[0]); xs.push_back(r[2]); }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());

    using YI = std::pair<double, double>;
    std::vector<std::vector<YI>> strip_yints(xs.size() > 0 ? xs.size() - 1 : 0);
    for (size_t i = 0; i + 1 < xs.size(); ++i) {
        const double sx0 = xs[i], sx1 = xs[i + 1];
        std::vector<YI> ys;
        for (const auto& r : in)
            if (r[0] <= sx0 && r[2] >= sx1) ys.push_back({r[1], r[3]});
        std::sort(ys.begin(), ys.end());
        std::vector<YI> merged;
        for (const auto& y : ys) {
            if (!merged.empty() && y.first <= merged.back().second)
                merged.back().second = std::max(merged.back().second, y.second);
            else
                merged.push_back(y);
        }
        strip_yints[i] = std::move(merged);
    }

    std::vector<Rect> out;
    size_t i = 0;
    while (i < strip_yints.size()) {
        size_t j = i;
        while (j + 1 < strip_yints.size() && strip_yints[j + 1] == strip_yints[i]) ++j;
        for (const auto& yi : strip_yints[i]) out.push_back({xs[i], yi.first, xs[j + 1], yi.second});
        i = j + 1;
    }
    return out;
}


using MetalEdge = std::pair<std::array<long, 3>, std::array<long, 3>>;


using LisdExtents = std::map<double, std::vector<std::pair<double, double>>>;

LisdExtents compute_lisd_fixed_extents(const Config& cfg, const NetOrders& no, const std::vector<Net>& nets) {
    LisdExtents out;
    const long ch = cfg.cell_height;
    const double x_off = cfg.x_offset;
    const double fin_pitch = cfg.pitch.at("fin");
    const double y_off = active_y_offset(cfg);
    const long ncol = (long)no.p_fins.size();
    for (long x = 1; x < ncol; x += 2) {
        const double cx = x_off + x * cfg.x_unit;
        const Net* pnet = find_net_by_name(nets, no.p_net[x]);
        const Net* nnet = find_net_by_name(nets, no.n_net[x]);
        if (pnet && pnet->pins.size() > 1) {
            auto [pmin, pmax] = p_fin_extent(ch, y_off, fin_pitch, no.p_fins[x]);
            if (pmax != pmin) out[cx].push_back({pmin, pmax});
        }
        if (nnet && nnet->pins.size() > 1) {
            auto [nmin, nmax] = n_fin_extent(y_off, fin_pitch, no.n_fins[x]);
            if (nmax != nmin) out[cx].push_back({nmin, nmax});
        }
    }
    return out;
}

std::pair<double, double> lisd_target_match(const LisdExtents& fixed, const std::vector<double>& x_positions,
                                            double y_min, double y_max) {
    double cand_min = y_min, cand_max = y_max;
    for (double x_pos : x_positions) {
        auto it = fixed.find(x_pos);
        if (it == fixed.end()) continue;
        for (const auto& ext : it->second) {
            if (ext.second > y_min && ext.first < y_max) {
                cand_min = std::min(cand_min, ext.first);
                cand_max = std::max(cand_max, ext.second);
            }
        }
    }
    return {cand_min, cand_max};
}

std::pair<double, double> lisd_clip_extent(const Config& cfg, const std::string& layer,
                                           double x_min, double x_max, double orig_y_min, double orig_y_max,
                                           double cand_min, double cand_max,
                                           const std::vector<Rect>& obstacle_bounds) {
    if (cand_min == orig_y_min && cand_max == orig_y_max) return {orig_y_min, orig_y_max};
    const long s2s = cfg.rules.at("spacing").at("S2S").at(layer).at(layer).get<long>();
    const long s2t = cfg.rules.at("spacing").at("S2T").at(layer).at(layer).get<long>();
    const long t2t = cfg.rules.at("spacing").at("T2T").at(layer).at(layer).get<long>();
    const double clearance = std::max({(double)s2s, (double)s2t, (double)t2t});
    double safe_min = cand_min, safe_max = cand_max;
    for (const auto& ob : obstacle_bounds) {
        const double poly_minx = ob[0], poly_miny = ob[1], poly_maxx = ob[2], poly_maxy = ob[3];
        if (poly_maxx <= x_min - clearance || poly_minx >= x_max + clearance) continue;
        if (poly_maxy > orig_y_min && poly_miny < orig_y_max) continue;
        if (poly_maxy <= orig_y_min) safe_min = std::max(safe_min, poly_maxy + clearance);
        else if (poly_miny >= orig_y_max) safe_max = std::min(safe_max, poly_miny - clearance);
    }
    return {std::min(safe_min, orig_y_min), std::max(safe_max, orig_y_max)};
}

void resolve_lisd_spans(const Config& cfg, const PreLayout& pre, const std::string& layer,
                        const std::vector<std::pair<std::array<long, 2>, std::array<long, 2>>>& spans,
                        const LisdExtents& lisd_fixed,
                        const std::function<void(int, Rect)>& add) {
    struct Desired { double x_min, x_max, orig_y_min, orig_y_max, cand_min, cand_max; };
    std::vector<Desired> desired;
    desired.reserve(spans.size());
    for (const auto& span : spans) {
        Rect raw = get_square(cfg, span.first[0], span.first[1], span.second[0], span.second[1], layer, pre);
        std::vector<double> xpos;
        if (span.first[0] == span.second[0]) xpos = {(double)span.first[0]};
        else xpos = {(double)span.first[0], (double)span.second[0]};
        auto cand = lisd_target_match(lisd_fixed, xpos, raw[1], raw[3]);
        desired.push_back({raw[0], raw[2], raw[1], raw[3], cand.first, cand.second});
    }
    std::vector<Rect> fixed_obstacles;
    for (const auto& kv : lisd_fixed) {
        const double hw = cfg.width(layer) / 2.0;
        for (const auto& ext : kv.second)
            fixed_obstacles.push_back({kv.first - hw, ext.first, kv.first + hw, ext.second});
    }
    for (size_t i = 0; i < desired.size(); ++i) {
        std::vector<Rect> obstacles = fixed_obstacles;
        for (size_t j = 0; j < desired.size(); ++j) {
            if (j == i) continue;
            obstacles.push_back({desired[j].x_min, desired[j].cand_min, desired[j].x_max, desired[j].cand_max});
        }
        auto final_y = lisd_clip_extent(cfg, layer, desired[i].x_min, desired[i].x_max,
                                         desired[i].orig_y_min, desired[i].orig_y_max,
                                         desired[i].cand_min, desired[i].cand_max, obstacles);
        add(gds_layer(cfg, layer), {desired[i].x_min, final_y.first, desired[i].x_max, final_y.second});
    }
}

void emit_bulk_active_contact_enclosure(
    const Config& cfg, const PreLayout& pre, const std::string& via, const std::string& lower,
    const std::string& upper, long x, long y, const std::function<void(int, Rect)>& add) {
    if (!bulk_planar(cfg) || via != "Cont") return;
    const bool active_to_m1 = (lower == "Active" && upper == "M1") ||
                              (lower == "M1" && upper == "Active");
    if (!active_to_m1) return;
    const double half = cfg.width(via) / 2.0 + cfg.opt_long("active_contact_enclosure");
    const long ov = cfg.opt_long("active_contact_pad_gate_overlap", 0);
    // Narrow the part of the pad that pokes past the diffusion so its side
    // wall keeps Gat.d to the next gate, instead of drawing the full square.
    const long oh = cfg.opt_long("active_contact_pad_outside_half", 0);
    auto clip = (oh > 0 && (y % (long)cfg.cell_height) != 0)
                    ? pre.active_band_at((double)x, (double)y)
                    : std::optional<std::pair<double, double>>();
    if (clip.has_value()) {
        const double lo = std::max((double)y - half, clip->first);
        const double hi = std::min((double)y + half, clip->second);
        if (hi > lo) add(gds_layer(cfg, "Active"), {x - half, lo, x + half, hi});
        if ((double)y + half > clip->second)
            add(gds_layer(cfg, "Active"), {x - (double)oh, clip->second, x + (double)oh, (double)y + half});
        if ((double)y - half < clip->first)
            add(gds_layer(cfg, "Active"), {x - (double)oh, (double)y - half, x + (double)oh, clip->first});
    } else {
        add(gds_layer(cfg, "Active"), {x - half, y - half, x + half, y + half});
    }
    if (ov <= 0 || (y % (long)cfg.cell_height) == 0) return;
    const double reach = cfg.x_unit - cfg.width("Gate") / 2.0 + (double)ov;
    auto band = pre.active_band_at((double)x, (double)y);
    if (!band.has_value()) {
        add(gds_layer(cfg, "Active"), {x - reach, y - half, x + reach, y + half});
        return;
    }
    if ((double)y + half > band->second)
        add(gds_layer(cfg, "Active"), {x - reach, band->second, x + reach, (double)y + half});
    if ((double)y - half < band->first)
        add(gds_layer(cfg, "Active"), {x - reach, (double)y - half, x + reach, band->first});
}

void emit_metals(const Config& cfg, const PreLayout& pre, const std::vector<MetalEdge>& metals,
                 std::map<int, std::vector<Rect>>& rects, double clamp_lo, double clamp_hi,
                 const LisdExtents* lisd_fixed = nullptr) {
    const auto& layers = cfg.routing_layers;
    auto add = [&](int gl, Rect r) { clamp_rect(r, clamp_lo, clamp_hi); rects[gl].push_back(r); };
    std::map<int, std::vector<std::pair<std::array<long, 2>, std::array<long, 2>>>> by_layer;
    for (const auto& e : metals) {
        const auto& a = e.first;
        const auto& b = e.second;
        if (a[2] == b[2]) {
            by_layer[a[2]].push_back({{a[0], a[1]}, {b[0], b[1]}});
        } else {
            const int lz = std::min(a[2], b[2]), uz = std::max(a[2], b[2]);
            const long x = a[0], y = a[1];
            const auto& via1 = cfg.upper_via.at(layers[lz]);
            const auto& via2 = cfg.lower_via.at(layers[uz]);
            if (via1.has_value() && !via1->empty() && via1 == via2) {
                add(gds_layer(cfg, *via1), get_square(cfg, x, y, x, y, *via1, pre));
                emit_bulk_active_contact_enclosure(cfg, pre, *via1, layers[lz], layers[uz], x, y, add);
                add(gds_layer(cfg, layers[uz]), get_square(cfg, x, y, x, y, layers[uz], pre));
                add(gds_layer(cfg, layers[lz]), get_square(cfg, x, y, x, y, layers[lz], pre));
                // A Cont landing on poly needs the poly to enclose it by
                // the Cnt.d minimum, wider than the bare gate square drawn
                // above. Draw the landing pad the reference cells use.
                if (*via1 == "Cont" && (layers[lz] == "Gate" || layers[uz] == "Gate")) {
                    const double pad = cfg.width("Cont") / 2.0 + 70.0;
                    add(gds_layer(cfg, "Gate"), {x - pad, y - pad, x + pad, y + pad});
                    // M1.d: a metal shape needs a minimum area, and the square
                    // drawn at a gate contact is well under it when nothing else
                    // on the net merges with it. Grow it along y, which is the
                    // free direction between the two diffusions.
                    // Minimum area is repaired once, after the union, on the
                    // shapes that actually fall short. Sizing it here grows every
                    // pad in the cell and regresses the ones already clear.
                    const double w = cfg.width("M1");
                    add(gds_layer(cfg, "M1"), {x - w / 2.0, y - w / 2.0, x + w / 2.0, y + w / 2.0});
                }
            } else {
                add(gds_layer(cfg, layers[uz]), get_square(cfg, x, y, x, y, layers[uz], pre));
                add(gds_layer(cfg, layers[lz]), get_square(cfg, x, y, x, y, layers[lz], pre));
            }

        }
    }
    for (auto& kv : by_layer) {
        const std::string& layer = layers[kv.first];
        auto spans = merge_collinear(kv.second);
        // LISD span repair is specific to the FinFET device stack. In IHP
        // bulk-planar mode Active is the contact layer, but it has no LISD rule set.
        if (lisd_fixed != nullptr && !bulk_planar(cfg) && layer == cfg.active_contact_layer && !spans.empty()) {
            resolve_lisd_spans(cfg, pre, layer, spans, *lisd_fixed, add);
            continue;
        }
        for (const auto& span : spans)
            add(gds_layer(cfg, layer), get_square(cfg, span.first[0], span.first[1], span.second[0], span.second[1], layer, pre));
    }
}

std::vector<int> walk_power_via_chain(const Config& cfg, const std::string& pl, int contact_z, int power_z) {
    std::vector<int> zs;
    const int step = (contact_z < power_z) ? 1 : -1;
    const auto& target_via = (step > 0) ? cfg.lower_via.at(pl) : cfg.upper_via.at(pl);
    std::vector<std::optional<std::string>> seen;
    auto seen_has = [&](const std::optional<std::string>& v) {
        return std::find(seen.begin(), seen.end(), v) != seen.end();
    };
    int z = contact_z;
    while ((step > 0 && z < power_z) || (step < 0 && power_z < z)) {
        const auto& v = (step > 0) ? cfg.upper_via.at(cfg.routing_layers[z]) : cfg.lower_via.at(cfg.routing_layers[z]);
        if (seen_has(v)) { z += step; continue; }
        seen.push_back(v); zs.push_back(z); z += step;
        if (v == target_via) { zs.push_back(power_z); break; }
    }
    return zs;
}

std::vector<MetalEdge> power_metals(const Config& cfg, const std::vector<Net>& nets) {
    std::vector<MetalEdge> out;
    const std::string& contact_layer = cfg.active_contact_layer;
    const int contact_z = cfg.routing_layer_index(contact_layer);
    for (const Net& net : nets) {
        if (!net.is_power || net.pins.size() < 2) continue;
        const Pin& contact = net.pins[0];
        const Pin& rail_pin = net.pins[1];
        if (rail_pin.points.empty() || contact.points.empty()) continue;
        const long rail_y = rail_pin.points[0].y;
        const Point* closest = &contact.points[0];
        for (const Point& p : contact.points)
            if (std::labs(p.y - rail_y) < std::labs(closest->y - rail_y)) closest = &p;
        out.push_back({{closest->x, closest->y, closest->z}, {closest->x, rail_y, closest->z}});

        for (const std::string& pl : cfg.power_layer) {
            if (pl == contact_layer) continue;
            const int power_z = cfg.routing_layer_index(pl);
            std::vector<int> zs = walk_power_via_chain(cfg, pl, contact_z, power_z);
            for (size_t i = 0; i + 1 < zs.size(); ++i)
                out.push_back({{closest->x, rail_y, zs[i]}, {closest->x, rail_y, zs[i + 1]}});
        }
    }
    return out;
}

using AddLayerFn = std::function<void(const std::string&, Rect, bool)>;

void emit_gates(const AddLayerFn& add, const Config& cfg, const NetOrders& no, long num_poly,
                double x_off, double gate_pitch, long ch) {
    // emit_bulk_active_gate_only: bulk mode keeps only the real gate
    // between source/drain columns and omits boundary dummy gates.
    const long begin = bulk_planar(cfg) ? 1 : 0;
    const long end = bulk_planar(cfg) ? std::max(1L, num_poly - 1) : num_poly;
    const double channel = gate_pitch - cfg.width("Gate") -
                           2.0 * (double)cfg.spacing_s2s("Active", "Gate").value_or(0);
    const double square = cfg.width("Cont") + 2.0 * (double)cfg.opt_long("active_contact_enclosure");
    const bool drop_bare = bulk_planar(cfg) && channel < square;
    const long ncol = (long)no.n_fins.size();
    // The reference cells overhang their diffusion by 180 and stop short of
    // both tap strips. Running the poly to the rails instead makes its crossing
    // of the tap read as a device, which pSD.i then cannot enclose.
    const double endcap = 180.0;
    const double y_off = active_y_offset(cfg);
    const double gate_row = cfg.cell_height / 2.0 + cfg.np_offset;
    const double extra = (double)cfg.opt_long("gate_extra_endcap", 0);
    const double lo = std::min(y_off - endcap - extra, gate_row - cfg.width("Gate"));
    const double hi = std::max((double)ch - y_off + endcap + extra, gate_row + cfg.width("Gate"));
    const double half_w = cfg.width("Gate") / 2.0;
    for (long x = begin; x < end; ++x) {
        if (drop_bare && 2 * x < ncol && no.n_fins[2 * x] == 0 && no.p_fins[2 * x] == 0) continue;
        const double cx = x_off + x * gate_pitch;
        // Two different nets on the two device rows. Draw a piece over each
        // diffusion instead of one bar, so the column can carry both. The gap
        // between them is the poly end to end spacing, and each piece still
        // overhangs its own diffusion by the endcap.
        const bool split = !cfg.opt_bool("require_matching_gate_nets", true) &&
                           2 * (size_t)x < no.n_net.size() &&
                           no.n_net[2 * x] != no.p_net[2 * x] &&
                           no.n_net[2 * x] != DUMMY_NET &&
                           no.p_net[2 * x] != DUMMY_NET;
        if (split) {
            const double n_top = y_off + (double)no.n_fins[2 * x] + endcap + extra;
            const double p_bot = (double)ch - y_off - (double)no.p_fins[2 * x] - endcap - extra;
            add("Gate", {cx - half_w, lo, cx + half_w, n_top}, false);
            add("Gate", {cx - half_w, p_bot, cx + half_w, hi}, false);
            continue;
        }
        add("Gate", {cx - half_w, lo, cx + half_w, hi}, false);
    }
}

void emit_fins(const AddLayerFn& add, const Config& cfg, long ch, double fin_pitch, double max_x) {
    if (bulk_planar(cfg)) return;
    const long num_fin = (long)(ch / fin_pitch);
    for (long i = 0; i < num_fin; ++i) {
        const double cy = fin_pitch / 2.0 + i * fin_pitch;
        add("fin", {0, cy - cfg.width("fin") / 2.0, max_x, cy + cfg.width("fin") / 2.0}, false);
    }
}

void emit_power_rails(const AddLayerFn& add, const Config& cfg, std::vector<GdsLabel>& labels,
                      long ch, double max_x) {
    const double left = 0.0;
    const double right = max_x;
    for (long r = 0; r <= 1; ++r) {
        const double rail = r * (double)ch;
        const std::string power = (r % 2 == 1) ? cfg.power_net : cfg.ground_net;
        for (const std::string& pl : cfg.power_layer) {
            const double pw = cfg.power_width(pl);
            add(pl, {left, rail - pw / 2.0, right, rail + pw / 2.0}, false);
            if (has_layer(cfg, cfg.power_layer.back()))
                labels.push_back({power, (left + right) / 2.0, rail, gds_layer(cfg, cfg.power_layer.back()), label_texttype(cfg, cfg.power_layer.back())});
        }
        if (bulk_planar(cfg)) add("Active", {left, rail - 150.0, right, rail + 150.0}, false);
    }
}

// The band between the two diffusions, which is what the well and the select
// layers are placed against. Built from the widest device the cell actually has
// on each row, not from the folding caps.
std::pair<double, double> diffusion_gap(const Config& cfg, const NetOrders& no, long ch) {
    long nmax = 0, pmax = 0;
    for (size_t i = 0; i < no.n_fins.size(); ++i) nmax = std::max(nmax, no.n_fins[i]);
    for (size_t i = 0; i < no.p_fins.size(); ++i) pmax = std::max(pmax, no.p_fins[i]);
    const double y_off = active_y_offset(cfg);
    return {y_off + (double)nmax, (double)ch - y_off - (double)pmax};
}

void emit_well_select(const AddLayerFn& add, const Config& cfg, const NetOrders& no,
                      std::vector<GdsLabel>& labels, long ch, double max_x) {
    auto [n_top, p_bot] = diffusion_gap(cfg, no, ch);
    const double nw = (double)cfg.opt_long("nwell_active_clearance", 310);
    // NW.d wants the well above the nmos diffusion, NW.c wants it below the
    // pmos diffusion. Sit in the middle of what both allow.
    const double lo = n_top + nw, hi = p_bot - nw;
    const double p_min_y = (lo <= hi) ? (lo + hi) / 2.0 : (long)(ch / 2.0 + cfg.np_offset);
    if (bulk_planar(cfg)) {
        // NW.e: the well has to enclose the tie, and the tie is the tap strip
        // that runs the whole cell. The library overhangs by exactly this.
        const double tie = (double)cfg.opt_long("nwell_tie_enclosure", 240);
        add("well", {-tie, p_min_y, max_x + tie, (double)ch + 390.0}, false);
        return;
    }
    add("well", {0, p_min_y, max_x, (double)ch}, false);
    add("Pselect", {0, p_min_y, max_x, (double)ch}, false);
    add("Nselect", {0, 0, max_x, p_min_y}, false);
    if (has_layer(cfg, "well")) labels.push_back({cfg.power_net, max_x / 2.0, p_min_y + (ch - p_min_y) / 2.0, gds_layer(cfg, "well"), label_texttype(cfg, "well")});
    if (has_layer(cfg, "P_SUB")) labels.push_back({cfg.ground_net, max_x + cfg.pitch.at("M1"), p_min_y + (ch - p_min_y) / 2.0, gds_layer(cfg, "P_SUB"), label_texttype(cfg, "P_SUB")});
}

void emit_gate_cuts(const AddLayerFn& add, const Config& cfg, const NetOrders& no,
                    long ch, double x_off, double max_x) {
    if (bulk_planar(cfg)) return;
    const double center_y = (long)(ch / 2.0 + cfg.np_offset);
    const long ncol = (long)no.p_fins.size();
    for (long x = 0; x < ncol; x += 2) {
        if (no.p_net[x] == "-" && no.n_net[x] == "-") {
            const double cx = x_off + x * cfg.x_unit;
            add("GCut", {cx - cfg.width("Gate") / 2.0 - off(cfg, "GCut", "Gate"), center_y - cfg.width("GCut") / 2.0,
                         cx + cfg.width("Gate") / 2.0 + off(cfg, "GCut", "Gate"), center_y + cfg.width("GCut") / 2.0}, false);
        }
    }
    for (long r = 0; r <= 1; ++r) {
        const double rail = r * (double)ch;
        add("GCut", {0, rail - cfg.width("GCut") / 2.0, max_x, rail + cfg.width("GCut") / 2.0}, false);
    }
}

void emit_active_regions(const AddLayerFn& add, const Config& cfg, const NetOrders& no,
                         long ch, double x_off, double fin_pitch, double y_off) {
    const long ncol = (long)no.p_fins.size();
    const double active_ext = active_gate_extension(cfg);
    for (long x = 2; x < ncol; x += 2) {
        const double cx = x_off + x * cfg.x_unit;
        const double mnx = cx - cfg.width("Gate") / 2.0 - active_ext;
        const double mxx = cx + cfg.width("Gate") / 2.0 + active_ext;
        auto [pmin, pmax] = p_fin_extent(ch, y_off, fin_pitch, no.p_fins[x]);
        if (pmax != pmin) add("Active", {mnx, pmin, mxx, pmax}, false);
        auto [nmin, nmax] = n_fin_extent(y_off, fin_pitch, no.n_fins[x]);
        if (nmax != nmin) add("Active", {mnx, nmin, mxx, nmax}, false);
    }
}

void emit_bulk_planar_psd(const AddLayerFn& add, const Config& cfg, const NetOrders& no,
                          long ch, double max_x) {
    if (!bulk_planar(cfg) || !has_layer(cfg, "pSD")) return;
    // The tap strip Activ runs the full cell width and is exempt from the
    // centring shift, so a pSD derived from the gate grid lands short of it
    // and pSD.c1 fires at the right edge (sg13g2_nand2_8). The reference
    // cells overhang the boundary by this much on both sides instead.
    const double over = (double)cfg.opt_long("psd_boundary_overhang", 70);
    const double left = -over;
    const double right = max_x + over;
    auto [n_top, p_bot] = diffusion_gap(cfg, no, ch);
    const double sel = (double)cfg.opt_long("psd_gate_clearance", 300);
    // pSD.j keeps it off the nfet gate, pSD.i makes it cover the pfet gate.
    const double lo = n_top + sel, hi = p_bot - sel;
    const double p_min_y = (lo <= hi) ? (lo + hi) / 2.0 : (long)(ch / 2.0 + cfg.np_offset);
    add("pSD", {left, p_min_y, right, (double)ch - 180.0}, false);
    add("pSD", {left, -180.0, right, 180.0}, false);
}

void emit_sdt_lisd_tracks(const AddLayerFn& add, const Config& cfg, const NetOrders& no,
                          const std::vector<Net>& nets, long ch, double x_off,
                          double fin_pitch, double y_off) {
    if (bulk_planar(cfg)) return;
    const long ncol = (long)no.p_fins.size();
    for (long x = 1; x < ncol; x += 2) {
        const double cx = x_off + x * cfg.x_unit;
        const Net* pnet = find_net_by_name(nets, no.p_net[x]);
        const Net* nnet = find_net_by_name(nets, no.n_net[x]);
        for (const char* lyr : {"SDT", "LISD"}) {
            const double hw = cfg.width(lyr) / 2.0;
            if (pnet && pnet->pins.size() > 1) {
                auto [pmin, pmax] = p_fin_extent(ch, y_off, fin_pitch, no.p_fins[x]);
                if (pmax != pmin) add(lyr, {cx - hw, pmin, cx + hw, pmax}, false);
            }
            if (nnet && nnet->pins.size() > 1) {
                auto [nmin, nmax] = n_fin_extent(y_off, fin_pitch, no.n_fins[x]);
                if (nmax != nmin) add(lyr, {cx - hw, nmin, cx + hw, nmax}, false);
            }
        }
    }
}

void build_device_geometry(const Config& cfg, const NetOrders& no, const std::vector<Net>& nets,
                           const PreLayout& pre, std::map<int, std::vector<Rect>>& rects,
                           std::vector<GdsLabel>& labels, double clamp_lo, double clamp_hi,
                           const LisdExtents& lisd_fixed) {
    const long ch = cfg.cell_height;
    const long num_poly = ((long)no.p_net.size() + 1) / 2;
    const double x_off = cfg.x_offset;
    const double gate_pitch = cfg.pitch.at("Gate"), fin_pitch = cfg.pitch.at("fin");
    const long site = cfg.opt_long("site_width");
    const double gate_span = num_poly * gate_pitch;
    const double max_x = (site > 0)
        ? std::ceil(gate_span / (double)site) * (double)site
        : gate_span;
    const double y_off = active_y_offset(cfg);
    AddLayerFn add = [&](const std::string& lname, Rect r, bool cut) {
        if (!has_layer(cfg, lname)) return;
        if (cut) clamp_rect(r, clamp_lo, clamp_hi);
        rects[gds_layer(cfg, lname)].push_back(r);
    };

    emit_gates(add, cfg, no, num_poly, x_off, gate_pitch, ch);
    emit_fins(add, cfg, ch, fin_pitch, max_x);
    emit_power_rails(add, cfg, labels, ch, max_x);
    add("BOUNDARY", {0, 0, max_x, (double)ch}, false);
    emit_well_select(add, cfg, no, labels, ch, max_x);
    emit_bulk_planar_psd(add, cfg, no, ch, max_x);
    emit_gate_cuts(add, cfg, no, ch, x_off, max_x);
    emit_active_regions(add, cfg, no, ch, x_off, fin_pitch, y_off);
    emit_sdt_lisd_tracks(add, cfg, no, nets, ch, x_off, fin_pitch, y_off);
    emit_metals(cfg, pre, power_metals(cfg, nets), rects, clamp_lo, clamp_hi, &lisd_fixed);
}

constexpr uint16_t GDS_HEADER = 0x0002;
constexpr uint16_t GDS_BGNLIB = 0x0102;
constexpr uint16_t GDS_LIBNAME = 0x0206;
constexpr uint16_t GDS_UNITS = 0x0305;
constexpr uint16_t GDS_ENDLIB = 0x0400;
constexpr uint16_t GDS_BGNSTR = 0x0502;
constexpr uint16_t GDS_STRNAME = 0x0606;
constexpr uint16_t GDS_ENDSTR = 0x0700;
constexpr uint16_t GDS_BOUNDARY = 0x0800;
constexpr uint16_t GDS_TEXT = 0x0C00;
constexpr uint16_t GDS_LAYER = 0x0D02;
constexpr uint16_t GDS_DATATYPE = 0x0E02;
constexpr uint16_t GDS_XY = 0x1003;
constexpr uint16_t GDS_ENDEL = 0x1100;
constexpr uint16_t GDS_TEXTTYPE = 0x1602;
constexpr uint16_t GDS_STRING = 0x1906;
constexpr int GDS_VERSION = 600;

void put16(std::vector<uint8_t>& b, uint16_t v) { b.push_back(v >> 8); b.push_back(v & 0xFF); }
void put32(std::vector<uint8_t>& b, int32_t v) {
    uint32_t u = (uint32_t)v;
    b.push_back(u >> 24); b.push_back((u >> 16) & 0xFF); b.push_back((u >> 8) & 0xFF); b.push_back(u & 0xFF);
}

constexpr int GDS_REAL8_EXP_BIAS = 64;
constexpr double GDS_REAL8_MANTISSA_SCALE = 72057594037927936.0;

void put_real8(std::vector<uint8_t>& b, double v) {
    if (v == 0.0) { for (int i = 0; i < 8; ++i) b.push_back(0); return; }
    uint64_t sign = 0;
    if (v < 0) { sign = 1; v = -v; }
    int exp = GDS_REAL8_EXP_BIAS;
    while (v >= 1.0) { v /= 16.0; ++exp; }
    while (v < 1.0 / 16.0) { v *= 16.0; --exp; }
    uint64_t mant = (uint64_t)(v * GDS_REAL8_MANTISSA_SCALE);
    uint64_t r = (sign << 63) | ((uint64_t)(exp & 0x7F) << 56) | (mant & 0x00FFFFFFFFFFFFFFULL);
    for (int i = 7; i >= 0; --i) b.push_back((r >> (i * 8)) & 0xFF);
}
void record(std::ofstream& f, uint16_t token, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> h;
    put16(h, (uint16_t)(4 + data.size()));
    put16(h, token);
    f.write((const char*)h.data(), h.size());
    if (!data.empty()) f.write((const char*)data.data(), data.size());
}
std::vector<uint8_t> str_data(const std::string& s) {
    std::vector<uint8_t> d(s.begin(), s.end());
    if (d.size() & 1) d.push_back(0);
    return d;
}

void write_gds_header(std::ofstream& f, const std::string& cell_name, const Config& cfg) {
    { std::vector<uint8_t> d; put16(d, GDS_VERSION); record(f, GDS_HEADER, d); }
    { std::vector<uint8_t> d; for (int i = 0; i < 12; ++i) put16(d, 0); record(f, GDS_BGNLIB, d); }
    record(f, GDS_LIBNAME, str_data(cell_name));
    const double dbu_nm = cfg.gds_database_unit_nm;
    double unit_um = dbu_nm / 1000.0, unit_m = dbu_nm * 1e-9;
    if (dbu_nm == 0.25) { unit_um = 2.5e-4; unit_m = 2.5e-10; }
    { std::vector<uint8_t> d; put_real8(d, unit_um); put_real8(d, unit_m); record(f, GDS_UNITS, d); }
    { std::vector<uint8_t> d; for (int i = 0; i < 12; ++i) put16(d, 0); record(f, GDS_BGNSTR, d); }
    record(f, GDS_STRNAME, str_data(cell_name));
}

void write_boundary_records(std::ofstream& f, const std::map<int, std::vector<Rect>>& rects,
                            double dbu_scale, int boundary_gds_layer) {
    for (const auto& kv : rects)
        for (const auto& r : kv.second) {
            record(f, GDS_BOUNDARY, {});
            { std::vector<uint8_t> d; put16(d, (uint16_t)kv.first); record(f, GDS_LAYER, d); }
            { std::vector<uint8_t> d; const uint16_t dt = (kv.first == boundary_gds_layer) ? 4 : 0; put16(d, dt); record(f, GDS_DATATYPE, d); }
            { std::vector<uint8_t> d;
              const int32_t lx = (int32_t)std::lround(r[0] * dbu_scale), ly = (int32_t)std::lround(r[1] * dbu_scale);
              const int32_t ux = (int32_t)std::lround(r[2] * dbu_scale), uy = (int32_t)std::lround(r[3] * dbu_scale);
              put32(d, lx); put32(d, ly); put32(d, ux); put32(d, ly);
              put32(d, ux); put32(d, uy); put32(d, lx); put32(d, uy);
              put32(d, lx); put32(d, ly); record(f, GDS_XY, d); }
            record(f, GDS_ENDEL, {});
        }
}

// A duplicate of an already-drawn shape on a fixed (layer, datatype), used
// for pin-marker layers (e.g. Metal1.pin, 8/2) that IHP's convention keeps
// separate from the drawing layer (8/0) but this emitter's `rects` map has
// no room for (same layer number, different datatype) side by side.
void write_extra_datatype_records(std::ofstream& f, const std::vector<Rect>& rects,
                                  int layer_num, int datatype, double dbu_scale) {
    for (const auto& r : rects) {
        record(f, GDS_BOUNDARY, {});
        { std::vector<uint8_t> d; put16(d, (uint16_t)layer_num); record(f, GDS_LAYER, d); }
        { std::vector<uint8_t> d; put16(d, (uint16_t)datatype); record(f, GDS_DATATYPE, d); }
        { std::vector<uint8_t> d;
          const int32_t lx = (int32_t)std::lround(r[0] * dbu_scale), ly = (int32_t)std::lround(r[1] * dbu_scale);
          const int32_t ux = (int32_t)std::lround(r[2] * dbu_scale), uy = (int32_t)std::lround(r[3] * dbu_scale);
          put32(d, lx); put32(d, ly); put32(d, ux); put32(d, ly);
          put32(d, ux); put32(d, uy); put32(d, lx); put32(d, uy);
          put32(d, lx); put32(d, ly); record(f, GDS_XY, d); }
        record(f, GDS_ENDEL, {});
    }
}

void write_label_records(std::ofstream& f, const std::vector<GdsLabel>& labels,
                         double dbu_scale) {
    for (const auto& lb : labels) {
        record(f, GDS_TEXT, {});
        { std::vector<uint8_t> d; put16(d, (uint16_t)lb.layer); record(f, GDS_LAYER, d); }
        { std::vector<uint8_t> d; put16(d, (uint16_t)lb.texttype); record(f, GDS_TEXTTYPE, d); }
        { std::vector<uint8_t> d; put32(d, (int32_t)std::lround(lb.x * dbu_scale)); put32(d, (int32_t)std::lround(lb.y * dbu_scale)); record(f, GDS_XY, d); }
        record(f, GDS_STRING, str_data(lb.text));
        record(f, GDS_ENDEL, {});
    }
}

void emit_external_label_fallback(const Config& cfg, const std::vector<Net>& nets,
                                  const RoutingResult& res,
                                  std::vector<GdsLabel>& labels, double pin_shift) {
    std::set<std::string> emitted;
    for (const auto& label : labels) emitted.insert(label.text);
    const int m1_gl = gds_layer(cfg, "M1");
    const int m1_z = cfg.routing_layer_index("M1");
    using Node = std::array<long, 3>;
    std::map<Node, std::vector<Node>> adj;
    for (const auto& e : res.metals) {
        adj[e.first].push_back(e.second);
        adj[e.second].push_back(e.first);
    }
    auto find_m1_in_net = [&](long x, long y, long z, Node& out, bool grid_only) {
        Node start{x, y, z};
        if (!adj.count(start)) return false;
        std::set<Node> seen{start};
        std::vector<Node> queue{start};
        for (size_t qi = 0; qi < queue.size(); ++qi) {
            const Node cur = queue[qi];
            auto it = adj.find(cur);
            if (cur[2] == m1_z) {
                long px = 0, py = 0;
                if (!grid_only) { out = cur; return true; }
                // A grid point on this node or on a wire leaving it, with the label clamped onto that metal.
                if (pin_grid_hit(cfg, pin_shift, cur[0], cur[1], cur[0], cur[1], px, py)) { out = Node{px, py, m1_z}; return true; }
                if (it != adj.end())
                    for (const auto& nxt : it->second)
                        if (nxt[2] == m1_z && pin_grid_hit(cfg, pin_shift, cur[0], cur[1], nxt[0], nxt[1], px, py)) {
                            out = Node{px, py, m1_z};
                            return true;
                        }
            }
            if (it == adj.end()) continue;
            for (const auto& nxt : it->second)
                if (seen.insert(nxt).second) queue.push_back(nxt);
        }
        return false;
    };
    for (const Net& net : nets) {
        if (!net.is_ext_pin || net.is_power || emitted.count(net.name)) continue;
        bool added = false;
        int used_pass = -1;
        // Pass 0 asks for Metal1 holding a pin grid point, so the label and the pin shape drawn on it land there.
        for (int pass = 0; pass < 2 && !added; ++pass) {
        for (const Pin& pin : net.pins) {
            for (const Point& point : pin.points) {
                Node hit{};
                if (!find_m1_in_net(point.x, point.y, point.z, hit, pass == 0)) continue;
                labels.push_back({net.name, static_cast<double>(hit[0]), static_cast<double>(hit[1]),
                                  m1_gl, label_texttype(cfg, "M1")});
                emitted.insert(net.name);
                added = true;
                used_pass = pass;
                break;
            }
            if (added) break;
        }
        }
        // The bulk-planar pin step checks the final pin geometry before writing.
        if (added && used_pass == 1 && pin_grid_active(cfg) && !bulk_planar(cfg))
            std::cerr << "WARNING: label of " << net.name << " has no Metal1 on the grid" << std::endl;
        if (added) continue;
        for (const Pin& pin : net.pins) {
            for (const Point& point : pin.points) {
                if (point.z < 0 || point.z >= static_cast<long>(cfg.routing_layers.size())) continue;
                labels.push_back({net.name, static_cast<double>(point.x), static_cast<double>(point.y),
                                  gds_layer(cfg, cfg.routing_layers[point.z]), label_texttype(cfg, cfg.routing_layers[point.z])});
                emitted.insert(net.name);
                added = true;
                break;
            }
            if (added) break;
        }
    }
}

void emit_via_enclosure_rects(const Config& cfg, const RoutingResult& res,
                              std::map<int, std::vector<Rect>>& rects,
                              double clamp_lo, double clamp_hi) {
    const long ch = cfg.cell_height;
    const auto& layers = cfg.routing_layers;
    auto add_cut = [&](int gl, Rect r) { clamp_rect(r, clamp_lo, clamp_hi); rects[gl].push_back(r); };
    for (const auto& v : res.via_enc) {
        const std::string& layer = layers[v.z];
        const auto& via = (v.level == 0) ? cfg.lower_via.at(layer) : cfg.upper_via.at(layer);
        if (!via.has_value() || via->empty()) continue;
        const long metal_width = (v.y % ch == 0 && cfg.is_power_layer(layer)) ? cfg.power_width(layer) : cfg.width(layer);
        const long metal_ex = ext_of(cfg, layer), via_width = cfg.width(*via);
        // Missing means "no extra enclosure required", not "value zero was
        // published", so default to 0 rather than throw when the pair is absent.
        // Do not read a missing entry as evidence the PDK has no such rule:
        // M2.c1 "Min. Metal2 endcap enclosure of Via1 = 0.05" is real, at
        // sg13g2_maximal.drc:2335, and 44 library Via1 cuts measure exactly 50.
        // While enclosure.V1 carried no M2 key, layer_enclosure returned 0 and
        // the M2 via-pad spacing clause was never emitted, which is where the
        // 165 gaps against a 210 rule came from.
        long enc = 0;
        const auto& enclosure_rules = cfg.rules.at("enclosure");
        if (enclosure_rules.contains(*via)) {
            const auto& via_rules = enclosure_rules.at(*via);
            if (via_rules.contains(layer)) enc = via_rules.at(layer).get<long>();
        }
        // Cross axis covers the cut and clears V1.c, so V1.c1's guard never fires.
        const long cross_enc = cfg.opt_long("via_cross_enclosure", 20);
        const long cross = (via_width > metal_width) ? via_width + 2 * cross_enc : metal_width;
        // Minimum area is repaired in the post-pass, on merged shapes, not here.
        const long metal_ex_a = metal_ex;
        long xe, ye, xmw, ymw;
        if (v.dir == HORIZONTAL) { xe = enc; ye = 0; xmw = metal_ex_a; ymw = cross; }
        else { xe = 0; ye = enc; xmw = cross; ymw = metal_ex_a; }
        const double ah = std::max(via_width / 2.0 + (double)std::max(xe, ye),
                                   metal_ex_a / 2.0);
        const double lx = (xe != 0) ? v.x - ah : v.x - xmw / 2.0;
        const double ux = (xe != 0) ? v.x + ah : v.x + xmw / 2.0;
        const double ly = (ye != 0) ? v.y - ah : v.y - ymw / 2.0;
        const double uy = (ye != 0) ? v.y + ah : v.y + ymw / 2.0;
        add_cut(gds_layer(cfg, layer), {lx, ly, ux, uy});
    }
}

using IRect = std::array<long, 4>;
using Pt = std::array<long, 2>;

// Integer copy of a drawn rectangle.
IRect to_irect(const Rect& r) {
    return {std::lround(r[0]), std::lround(r[1]), std::lround(r[2]), std::lround(r[3])};
}

// Drawn rectangle from an integer one.
Rect to_rect(const IRect& r) { return {(double)r[0], (double)r[1], (double)r[2], (double)r[3]}; }

// True when the rectangles overlap or share a boundary point.
bool ir_touch(const IRect& a, const IRect& b) {
    return a[0] <= b[2] && b[0] <= a[2] && a[1] <= b[3] && b[1] <= a[3];
}

// True when the rectangles share interior area.
bool ir_overlap(const IRect& a, const IRect& b) {
    return a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3];
}

// Euclidean distance between two rectangles, zero when they touch.
double ir_gap(const IRect& a, const IRect& b) {
    const long dx = std::max(0L, std::max(a[0] - b[2], b[0] - a[2]));
    const long dy = std::max(0L, std::max(a[1] - b[3], b[1] - a[3]));
    return std::sqrt((double)dx * (double)dx + (double)dy * (double)dy);
}

// Rectangle grown by e on every side.
IRect ir_grow(const IRect& r, long e) { return {r[0] - e, r[1] - e, r[2] + e, r[3] + e}; }

// Smallest rectangle holding both.
IRect ir_hull(const IRect& a, const IRect& b) {
    return {std::min(a[0], b[0]), std::min(a[1], b[1]), std::max(a[2], b[2]), std::max(a[3], b[3])};
}

// The area between two separated rectangles, where a notch between them would sit.
IRect ir_bridge(const IRect& a, const IRect& b) {
    long x0 = std::max(a[0], b[0]), x1 = std::min(a[2], b[2]);
    long y0 = std::max(a[1], b[1]), y1 = std::min(a[3], b[3]);
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    if (x0 == x1) { --x0; ++x1; }
    if (y0 == y1) { --y0; ++y1; }
    return {x0, y0, x1, y1};
}

// True when the union of rs covers q.
bool ir_covered(const IRect& q, const std::vector<IRect>& rs) {
    if (q[0] >= q[2] || q[1] >= q[3]) return true;
    std::vector<IRect> near;
    for (const IRect& r : rs) if (ir_overlap(r, q)) near.push_back(r);
    std::vector<long> xs{q[0], q[2]}, ys{q[1], q[3]};
    for (const IRect& r : near) {
        if (r[0] > q[0] && r[0] < q[2]) xs.push_back(r[0]);
        if (r[2] > q[0] && r[2] < q[2]) xs.push_back(r[2]);
        if (r[1] > q[1] && r[1] < q[3]) ys.push_back(r[1]);
        if (r[3] > q[1] && r[3] < q[3]) ys.push_back(r[3]);
    }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    for (size_t i = 0; i + 1 < xs.size(); ++i)
        for (size_t j = 0; j + 1 < ys.size(); ++j) {
            const long mx = xs[i] + xs[i + 1], my = ys[j] + ys[j + 1];
            bool in = false;
            for (const IRect& r : near)
                if (2 * r[0] < mx && mx < 2 * r[2] && 2 * r[1] < my && my < 2 * r[3]) { in = true; break; }
            if (!in) return false;
        }
    return true;
}

// Rectangles of one layer with the connected piece each belongs to.
struct Pieces {
    std::vector<IRect> r;
    std::vector<int> comp;
    // Labels connected pieces, rectangles joining when they touch.
    void relabel() {
        std::vector<int> parent(r.size());
        for (size_t i = 0; i < r.size(); ++i) parent[i] = (int)i;
        auto find = [&](int a) { while (parent[a] != a) { parent[a] = parent[parent[a]]; a = parent[a]; } return a; };
        for (size_t i = 0; i < r.size(); ++i)
            for (size_t j = i + 1; j < r.size(); ++j)
                if (ir_touch(r[i], r[j])) {
                    const int a = find((int)i), b = find((int)j);
                    if (a != b) parent[a] = b;
                }
        comp.assign(r.size(), 0);
        for (size_t i = 0; i < r.size(); ++i) comp[i] = find((int)i);
    }
    // Piece holding the point, or -1.
    int at(long x, long y) const {
        for (size_t i = 0; i < r.size(); ++i)
            if (r[i][0] <= x && x <= r[i][2] && r[i][1] <= y && y <= r[i][3]) return comp[i];
        return -1;
    }
    // A new shape may touch only its own piece and keeps the space from every other rectangle, unless its own metal fills the gap.
    bool clear(const IRect& c, int own, long space) const {
        std::vector<IRect> mine;
        for (size_t i = 0; i < r.size(); ++i)
            if (comp[i] == own) mine.push_back(r[i]);
        mine.push_back(c);
        for (size_t i = 0; i < r.size(); ++i) {
            if (ir_touch(c, r[i])) {
                if (comp[i] != own) return false;
                continue;
            }
            if (ir_gap(c, r[i]) >= (double)space) continue;
            if (comp[i] != own) return false;
            if (!ir_covered(ir_bridge(c, r[i]), mine)) return false;
        }
        return true;
    }
    // Like clear, but a notch against the own piece is returned as fills to draw instead of being refused.
    bool clear_fill(const IRect& c, int own, long space, std::vector<IRect>& fills) const {
        fills.clear();
        std::vector<IRect> mine;
        for (size_t i = 0; i < r.size(); ++i)
            if (comp[i] == own) mine.push_back(r[i]);
        mine.push_back(c);
        for (size_t i = 0; i < r.size(); ++i) {
            if (ir_touch(c, r[i])) {
                if (comp[i] != own) return false;
                continue;
            }
            if (ir_gap(c, r[i]) >= (double)space) continue;
            if (comp[i] != own) return false;
            const IRect b = ir_bridge(c, r[i]);
            if (!ir_covered(b, mine)) fills.push_back(b);
        }
        for (const IRect& f : fills)
            for (size_t i = 0; i < r.size(); ++i)
                if (comp[i] != own && (ir_touch(f, r[i]) || ir_gap(f, r[i]) < (double)space)) return false;
        return true;
    }
    // True when some rectangle of the piece touches c.
    bool touches(const IRect& c, int own) const {
        for (size_t i = 0; i < r.size(); ++i)
            if (comp[i] == own && ir_touch(c, r[i])) return true;
        return false;
    }
    void add(const IRect& c, int own) { r.push_back(c); comp.push_back(own); }
};

// First multiple of p at or above v.
long first_mult(long v, long p) {
    long q = v / p;
    if (q * p < v) ++q;
    return q * p;
}

struct PinPick { IRect r{{0, 0, 0, 0}}; bool found = false; long margin = -1; long area = -1; };

// Largest rectangle inside rs holding a routing grid point, or holding (px, py) when need_pt.
PinPick best_rect(const std::vector<IRect>& rs, long gx, long gy, bool need_pt, long px, long py, long cap) {
    PinPick best;
    std::vector<long> xs, ys;
    for (const IRect& r : rs) {
        if (r[0] >= r[2] || r[1] >= r[3]) continue;
        xs.push_back(r[0]); xs.push_back(r[2]); ys.push_back(r[1]); ys.push_back(r[3]);
    }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    const int nx = (int)xs.size() - 1, ny = (int)ys.size() - 1;
    if (nx <= 0 || ny <= 0) return best;
    std::vector<char> fill((size_t)nx * ny, 0);
    for (const IRect& r : rs) {
        if (r[0] >= r[2] || r[1] >= r[3]) continue;
        const int i0 = (int)(std::lower_bound(xs.begin(), xs.end(), r[0]) - xs.begin());
        const int i1 = (int)(std::lower_bound(xs.begin(), xs.end(), r[2]) - xs.begin());
        const int j0 = (int)(std::lower_bound(ys.begin(), ys.end(), r[1]) - ys.begin());
        const int j1 = (int)(std::lower_bound(ys.begin(), ys.end(), r[3]) - ys.begin());
        for (int i = i0; i < i1; ++i)
            for (int j = j0; j < j1; ++j) fill[(size_t)i * ny + j] = 1;
    }
    std::vector<char> ok((size_t)ny, 1);
    for (int a = 0; a < nx; ++a) {
        std::fill(ok.begin(), ok.end(), 1);
        for (int b = a + 1; b <= nx; ++b) {
            for (int j = 0; j < ny; ++j) ok[j] = ok[j] && fill[(size_t)(b - 1) * ny + j];
            const long x0 = xs[a], x1 = xs[b];
            int j = 0;
            while (j < ny) {
                if (!ok[j]) { ++j; continue; }
                int k = j;
                while (k < ny && ok[k]) ++k;
                const long y0 = ys[j], y1 = ys[k];
                const long area = (x1 - x0) * (y1 - y0);
                long margin = -1;
                if (need_pt) {
                    if (x0 <= px && px <= x1 && y0 <= py && py <= y1) margin = 0;
                } else {
                    long mx = -1, my = -1;
                    for (long g = first_mult(x0, gx); g <= x1; g += gx) mx = std::max(mx, std::min(g - x0, x1 - g));
                    for (long g = first_mult(y0, gy); g <= y1; g += gy) my = std::max(my, std::min(g - y0, y1 - g));
                    if (mx >= 0 && my >= 0) margin = std::min(std::min(mx, my), cap);
                }
                if (margin >= 0 && (margin > best.margin || (margin == best.margin && area > best.area))) {
                    best.found = true;
                    best.margin = margin;
                    best.area = area;
                    best.r = {x0, y0, x1, y1};
                }
                j = k;
            }
        }
    }
    return best;
}

// Area of q left uncovered by the union of rs.
long uncovered_area(const IRect& q, const std::vector<IRect>& rs) {
    std::vector<IRect> near;
    for (const IRect& r : rs) if (ir_overlap(r, q)) near.push_back(r);
    std::vector<long> xs{q[0], q[2]}, ys{q[1], q[3]};
    for (const IRect& r : near) {
        if (r[0] > q[0] && r[0] < q[2]) xs.push_back(r[0]);
        if (r[2] > q[0] && r[2] < q[2]) xs.push_back(r[2]);
        if (r[1] > q[1] && r[1] < q[3]) ys.push_back(r[1]);
        if (r[3] > q[1] && r[3] < q[3]) ys.push_back(r[3]);
    }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    long a = 0;
    for (size_t i = 0; i + 1 < xs.size(); ++i)
        for (size_t j = 0; j + 1 < ys.size(); ++j) {
            const long mx = xs[i] + xs[i + 1], my = ys[j] + ys[j + 1];
            bool in = false;
            for (const IRect& r : near)
                if (2 * r[0] < mx && mx < 2 * r[2] && 2 * r[1] < my && my < 2 * r[3]) { in = true; break; }
            if (!in) a += (xs[i + 1] - xs[i]) * (ys[j + 1] - ys[j]);
        }
    return a;
}

struct PinExt { IRect r{{0, 0, 0, 0}}; int piece = -1; long margin = -1; long added = -1; };

// One rectangle of the port net's own Metal1 holding a pin grid point, widest margin first, then least added area.
PinExt grid_extension(const Pieces& m1, const std::set<int>& own, long gx, long gy, long cap, long space, long wmin,
                      long wide_w, long wide_s, long reach, long edge_reach, long max_x, long ch) {
    PinExt best;
    if (gx <= 0 || gy <= 0 || own.empty()) return best;
    std::vector<IRect> mine, other;
    std::vector<long> xe, ye;
    for (size_t i = 0; i < m1.r.size(); ++i) {
        const IRect& r = m1.r[i];
        if (!own.count(m1.comp[i])) { other.push_back(r); continue; }
        mine.push_back(r);
        xe.push_back(r[0]); xe.push_back(r[2]);
        ye.push_back(r[1]); ye.push_back(r[3]);
    }
    auto gap_other = [&](const IRect& c) {
        double g = 1e18;
        for (const IRect& o : other) g = std::min(g, ir_gap(c, o));
        return g;
    };
    auto order = [](std::vector<long>& v, bool down) {
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
        if (down) std::reverse(v.begin(), v.end());
    };
    // Half the space off the left and right cell edges, so an abutting cell still sees the full space.
    const long lo_x = space / 2, hi_x = max_x - space / 2;
    std::vector<long> margins;
    for (long m : {cap, cap * 3 / 4, cap / 2, cap / 4, cap / 8, 5L}) {
        m = m / 5 * 5;
        if (m >= 5 && (margins.empty() || m < margins.back())) margins.push_back(m);
    }
    for (long m : margins) {
        for (long x = gx; x < max_x; x += gx)
            for (long y = gy; y < ch; y += gy) {
                const IRect s{{x - m, y - m, x + m, y + m}};
                if (s[0] < lo_x || s[2] > hi_x || s[1] < 0 || s[3] > ch) continue;
                double d_own = 1e18;
                for (const IRect& r : mine) d_own = std::min(d_own, ir_gap(s, r));
                if (d_own > (double)reach || gap_other(s) < (double)space) continue;
                // Edges grow outward from the grid square, so the innermost loop stops at the first rectangle that
                // leaves the cell or nears another piece, as every larger one does too.
                std::vector<long> xl{s[0]}, xr{s[2]}, yb{s[1]}, yt{s[3]};
                if (s[2] - wmin < s[0]) xl.push_back(s[2] - wmin);
                if (s[0] + wmin > s[2]) xr.push_back(s[0] + wmin);
                if (s[3] - wmin < s[1]) yb.push_back(s[3] - wmin);
                if (s[1] + wmin > s[3]) yt.push_back(s[1] + wmin);
                for (long e : xe) {
                    if (e < s[0] && e >= s[0] - edge_reach) xl.push_back(e);
                    if (e > s[2] && e <= s[2] + edge_reach) xr.push_back(e);
                }
                for (long e : ye) {
                    if (e < s[1] && e >= s[1] - edge_reach) yb.push_back(e);
                    if (e > s[3] && e <= s[3] + edge_reach) yt.push_back(e);
                }
                order(xl, true);
                order(xr, false);
                order(yb, true);
                order(yt, false);
                for (long a : xl)
                    for (long b : xr)
                        for (long c : yb)
                            for (long d : yt) {
                                const IRect rc{{a, c, b, d}};
                                if (a < lo_x || b > hi_x || c < 0 || d > ch) break;
                                if (gap_other(rc) < (double)space) break;
                                if (b - a < wmin || d - c < wmin) continue;
                                // One piece of the net, and every rectangle of it touched meets the new one across a
                                // full side of either, so the join is never narrower than the Metal1 width.
                                int p = -1;
                                bool ok = true, overlap = false;
                                for (size_t i = 0; i < m1.r.size() && ok; ++i) {
                                    const IRect& o = m1.r[i];
                                    if (!own.count(m1.comp[i]) || !ir_touch(rc, o)) continue;
                                    if (p >= 0 && m1.comp[i] != p) ok = false;
                                    p = m1.comp[i];
                                    if (ir_overlap(rc, o)) overlap = true;
                                    const long jx0 = std::max(a, o[0]), jx1 = std::min(b, o[2]);
                                    const long jy0 = std::max(c, o[1]), jy1 = std::min(d, o[3]);
                                    if (!((jx0 == a && jx1 == b) || (jy0 == c && jy1 == d) ||
                                          (jx0 == o[0] && jx1 == o[2] && jy0 == o[1] && jy1 == o[3]))) ok = false;
                                }
                                if (!ok || !overlap || !m1.clear(rc, p, space)) continue;
                                if (b - a > wide_w && d - c > wide_w && gap_other(rc) < (double)wide_s) continue;
                                std::vector<IRect> pr;
                                for (size_t i = 0; i < m1.r.size(); ++i)
                                    if (m1.comp[i] == p) pr.push_back(m1.r[i]);
                                const long added = uncovered_area(rc, pr);
                                if (best.piece < 0 || added < best.added) best = PinExt{rc, p, m, added};
                            }
            }
        if (best.piece >= 0) return best;
    }
    return best;
}

// True when rc meets o across a full side of either rectangle, a side of o counting only when it is at least wmin long.
bool full_side_join(const IRect& rc, const IRect& o, long wmin) {
    const long jx0 = std::max(rc[0], o[0]), jx1 = std::min(rc[2], o[2]);
    const long jy0 = std::max(rc[1], o[1]), jy1 = std::min(rc[3], o[3]);
    if ((jx0 == rc[0] && jx1 == rc[2]) || (jy0 == rc[1] && jy1 == rc[3])) return true;
    if (jx0 == o[0] && jx1 == o[2] && jy0 == o[1] && jy1 == o[3]) return true;
    return (jx0 == o[0] && jx1 == o[2] && o[2] - o[0] >= wmin) || (jy0 == o[1] && jy1 == o[3] && o[3] - o[1] >= wmin);
}

struct PinExt2 { IRect r1{{0, 0, 0, 0}}, r2{{0, 0, 0, 0}}; int piece = -1; long margin = -1; long added = -1; };

// Two rectangles of the port net's own Metal1, the first joined to the net and the second to the first holding a grid point, each under the single-rectangle rules, widest margin first, then least added area.
PinExt2 grid_extension2(const Pieces& m1, const std::set<int>& own, long gx, long gy, long cap, long space, long wmin,
                        long wide_w, long wide_s, long reach, long edge_reach, long max_x, long ch) {
    PinExt2 best;
    if (gx <= 0 || gy <= 0 || own.empty()) return best;
    std::vector<IRect> mine, other;
    std::map<int, std::vector<IRect>> piece_rects;
    for (size_t i = 0; i < m1.r.size(); ++i) {
        if (!own.count(m1.comp[i])) { other.push_back(m1.r[i]); continue; }
        mine.push_back(m1.r[i]);
        piece_rects[m1.comp[i]].push_back(m1.r[i]);
    }
    const long lo_x = space / 2, hi_x = max_x - space / 2;
    std::vector<long> margins;
    for (long m : {cap, cap * 3 / 4, cap / 2, cap / 4, cap / 8, 5L}) {
        m = m / 5 * 5;
        if (m >= 5 && (margins.empty() || m < margins.back())) margins.push_back(m);
    }
    auto uniq = [](std::vector<long>& v, long lo, long hi) {
        v.erase(std::remove_if(v.begin(), v.end(), [&](long e) { return e < lo || e > hi; }), v.end());
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
    };
    struct First { IRect r; int p; long added; int clear; };
    for (long m : margins) {
        for (long x = gx; x < max_x; x += gx)
            for (long y = gy; y < ch; y += gy) {
                const IRect s{{x - m, y - m, x + m, y + m}};
                if (s[0] < lo_x || s[2] > hi_x || s[1] < 0 || s[3] > ch) continue;
                double d_own = 1e18;
                for (const IRect& r : mine) d_own = std::min(d_own, ir_gap(s, r));
                if (d_own > (double)reach) continue;
                // Both rectangles stay inside this window, so only metal near it can matter.
                const IRect win{{std::max(lo_x, s[0] - edge_reach), std::max(0L, s[1] - edge_reach),
                                 std::min(hi_x, s[2] + edge_reach), std::min(ch, s[3] + edge_reach)}};
                std::vector<IRect> oth, in;
                std::vector<int> in_p;
                for (const IRect& o : other)
                    if (ir_gap(win, o) < (double)std::max(space, wide_s)) oth.push_back(o);
                for (size_t i = 0; i < m1.r.size(); ++i)
                    if (own.count(m1.comp[i]) && ir_touch(win, m1.r[i])) { in.push_back(m1.r[i]); in_p.push_back(m1.comp[i]); }
                auto gap_other = [&](const IRect& c) {
                    double g = 1e18;
                    for (const IRect& o : oth) g = std::min(g, ir_gap(c, o));
                    return g;
                };
                if (gap_other(s) < (double)space) continue;
                // At least wmin wide, inside the cell, the space from every other net, and M1.e beside metal wider than wide_w.
                auto legal = [&](const IRect& c) {
                    if (c[2] - c[0] < wmin || c[3] - c[1] < wmin) return false;
                    if (c[0] < lo_x || c[2] > hi_x || c[1] < 0 || c[3] > ch) return false;
                    const double g = gap_other(c);
                    return g >= (double)space && !(c[2] - c[0] > wide_w && c[3] - c[1] > wide_w && g < (double)wide_s);
                };
                // Piece of the own rectangles rc touches, each met across a full side, -1 when none, -2 when two pieces or a partial side.
                auto joined = [&](const IRect& rc, bool& overlap) {
                    int p = -1;
                    overlap = false;
                    for (size_t k = 0; k < in.size(); ++k) {
                        if (!ir_touch(rc, in[k])) continue;
                        if ((p >= 0 && in_p[k] != p) || !full_side_join(rc, in[k], wmin)) return -2;
                        p = in_p[k];
                        if (ir_overlap(rc, in[k])) overlap = true;
                    }
                    return p;
                };
                // The first rectangle takes the edges of the square and of the net's Metal1 near it, and the space line off each other net, each also moved by the Metal1 width.
                IRect core = s;
                for (const IRect& r : in)
                    if (ir_gap(s, r) <= (double)reach) core = ir_hull(core, r);
                core = ir_grow(core, wmin);
                std::vector<long> xs{s[0], s[2]}, ys{s[1], s[3]};
                for (const IRect& r : in) {
                    xs.push_back(r[0]);
                    xs.push_back(r[2]);
                    ys.push_back(r[1]);
                    ys.push_back(r[3]);
                }
                for (const IRect& o : oth) {
                    xs.push_back(o[0] - space);
                    xs.push_back(o[2] + space);
                    ys.push_back(o[1] - space);
                    ys.push_back(o[3] + space);
                }
                for (size_t k = 0, n = xs.size(); k < n; ++k) {
                    xs.push_back(xs[k] - wmin);
                    xs.push_back(xs[k] + wmin);
                }
                for (size_t k = 0, n = ys.size(); k < n; ++k) {
                    ys.push_back(ys[k] - wmin);
                    ys.push_back(ys[k] + wmin);
                }
                uniq(xs, std::max(core[0], win[0]), std::min(core[2], win[2]));
                uniq(ys, std::max(core[1], win[1]), std::min(core[3], win[3]));
                std::vector<First> firsts;
                for (size_t a = 0; a < xs.size(); ++a)
                    for (size_t b = a + 1; b < xs.size(); ++b) {
                        if (xs[b] - xs[a] < wmin) continue;
                        bool near_x = false;
                        for (const IRect& r : in)
                            if (xs[a] < r[2] && r[0] < xs[b]) near_x = true;
                        if (!near_x) continue;
                        for (size_t c = 0; c < ys.size(); ++c)
                            for (size_t d = c + 1; d < ys.size(); ++d) {
                                if (ys[d] - ys[c] < wmin) continue;
                                const IRect rc{{xs[a], ys[c], xs[b], ys[d]}};
                                bool overlap = false;
                                const int p = joined(rc, overlap);
                                if (p < 0 || !overlap || !legal(rc)) continue;
                                firsts.push_back(First{rc, p, uncovered_area(rc, piece_rects[p]), -1});
                            }
                    }
                std::stable_sort(firsts.begin(), firsts.end(), [](const First& u, const First& v) { return u.added < v.added; });
                for (First& f : firsts) {
                    if (best.piece >= 0 && f.added >= best.added) break;
                    // The second rectangle holds the square and takes its sides, the first one's edges, or a coordinate between them.
                    std::vector<long> xl, xr, yb, yt;
                    for (long e : xs) {
                        if (e >= s[2] - wmin && e <= s[0]) xl.push_back(e);
                        if (e >= s[2] && e <= s[0] + wmin) xr.push_back(e);
                    }
                    for (long e : ys) {
                        if (e >= s[3] - wmin && e <= s[1]) yb.push_back(e);
                        if (e >= s[3] && e <= s[1] + wmin) yt.push_back(e);
                    }
                    for (long e : {f.r[0], f.r[2]}) {
                        if (e <= s[0]) xl.push_back(e);
                        if (e >= s[2]) xr.push_back(e);
                    }
                    for (long e : {f.r[1], f.r[3]}) {
                        if (e <= s[1]) yb.push_back(e);
                        if (e >= s[3]) yt.push_back(e);
                    }
                    uniq(xl, win[0], win[2]);
                    uniq(xr, win[0], win[2]);
                    uniq(yb, win[1], win[3]);
                    uniq(yt, win[1], win[3]);
                    std::vector<IRect> pr = piece_rects[f.p];
                    pr.push_back(f.r);
                    for (long a : xl)
                        for (long b : xr)
                            for (long c : yb)
                                for (long d : yt) {
                                    const IRect r2{{a, c, b, d}};
                                    if (!ir_overlap(r2, f.r) || !full_side_join(r2, f.r, wmin) || !legal(r2)) continue;
                                    bool overlap = false;
                                    const int p2 = joined(r2, overlap);
                                    if (p2 == -2 || (p2 >= 0 && p2 != f.p)) continue;
                                    const long added = f.added + uncovered_area(r2, pr);
                                    if (best.piece >= 0 && added >= best.added) continue;
                                    // The notch check of each rectangle, the second one against the piece already grown by the first.
                                    if (f.clear < 0) f.clear = m1.clear(f.r, f.p, space) ? 1 : 0;
                                    if (f.clear == 0) continue;
                                    Pieces m2 = m1;
                                    m2.add(f.r, f.p);
                                    if (!m2.clear(r2, f.p, space)) continue;
                                    best = PinExt2{f.r, r2, f.p, m, added};
                                }
                }
            }
        if (best.piece >= 0) return best;
    }
    return best;
}

// Merged rectangle outlines. Hole-fractured pieces are rejoined by zero-width GDS bridges.
std::vector<std::vector<Pt>> merged_polygons(const std::vector<IRect>& in) {
    std::vector<std::vector<Pt>> out;
    std::vector<IRect> rs;
    for (const IRect& r : in) if (r[0] < r[2] && r[1] < r[3]) rs.push_back(r);
    if (rs.empty()) return out;
    std::vector<long> xs, ys;
    for (const IRect& r : rs) { xs.push_back(r[0]); xs.push_back(r[2]); ys.push_back(r[1]); ys.push_back(r[3]); }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    const int nx = (int)xs.size() - 1, ny = (int)ys.size() - 1;
    auto at = [&](int i, int j) { return (size_t)i * (size_t)ny + (size_t)j; };
    auto cut_at = [&](int i, int j) { return (size_t)i * (size_t)(ny + 1) + (size_t)j; };
    std::vector<char> fill((size_t)nx * ny, 0);
    for (const IRect& r : rs) {
        const int i0 = (int)(std::lower_bound(xs.begin(), xs.end(), r[0]) - xs.begin());
        const int i1 = (int)(std::lower_bound(xs.begin(), xs.end(), r[2]) - xs.begin());
        const int j0 = (int)(std::lower_bound(ys.begin(), ys.end(), r[1]) - ys.begin());
        const int j1 = (int)(std::lower_bound(ys.begin(), ys.end(), r[3]) - ys.begin());
        for (int i = i0; i < i1; ++i)
            for (int j = j0; j < j1; ++j) fill[at(i, j)] = 1;
    }
    std::vector<char> cut((size_t)nx * (ny + 1), 0);
    std::vector<char> vcut((size_t)(nx + 1) * ny, 0);
    auto vcut_at = [&](int i, int j) { return (size_t)i * (size_t)ny + (size_t)j; };
    std::vector<int> comp((size_t)nx * ny, -1);
    int ncomp = 0;
    auto label = [&]() {
        std::fill(comp.begin(), comp.end(), -1);
        ncomp = 0;
        std::vector<std::pair<int, int>> st;
        for (int i = 0; i < nx; ++i)
            for (int j = 0; j < ny; ++j) {
                if (!fill[at(i, j)] || comp[at(i, j)] >= 0) continue;
                comp[at(i, j)] = ncomp;
                st.push_back({i, j});
                while (!st.empty()) {
                    const int a = st.back().first, b = st.back().second;
                    st.pop_back();
                    auto visit = [&](int u, int v) {
                        if (u < 0 || v < 0 || u >= nx || v >= ny) return;
                        if (!fill[at(u, v)] || comp[at(u, v)] >= 0) return;
                        comp[at(u, v)] = ncomp;
                        st.push_back({u, v});
                    };
                    if (!vcut[vcut_at(a, b)]) visit(a - 1, b);
                    if (!vcut[vcut_at(a + 1, b)]) visit(a + 1, b);
                    if (b > 0 && !cut[cut_at(a, b)]) visit(a, b - 1);
                    if (b + 1 < ny && !cut[cut_at(a, b + 1)]) visit(a, b + 1);
                }
                ++ncomp;
            }
    };
    label();
    const auto original_comp = comp;
    for (int iter = 0; iter < 100000; ++iter) {
        label();
        std::vector<std::array<int, 4>> bb((size_t)ncomp, std::array<int, 4>{{nx, ny, -1, -1}});
        std::vector<long> cnt((size_t)ncomp, 0);
        for (int i = 0; i < nx; ++i)
            for (int j = 0; j < ny; ++j) {
                const int c = comp[at(i, j)];
                if (c < 0) continue;
                auto& b = bb[(size_t)c];
                b[0] = std::min(b[0], i);
                b[1] = std::min(b[1], j);
                b[2] = std::max(b[2], i);
                b[3] = std::max(b[3], j);
                ++cnt[(size_t)c];
            }
        bool opened = false;
        for (int c = 0; c < ncomp && !opened; ++c) {
            const std::array<int, 4> b = bb[(size_t)c];
            if ((long)(b[2] - b[0] + 1) * (long)(b[3] - b[1] + 1) == cnt[(size_t)c]) continue;
            const int W = b[2] - b[0] + 3, Hh = b[3] - b[1] + 3;
            auto own = [&](int u, int v) {
                const int i = b[0] - 1 + u, j = b[1] - 1 + v;
                return i >= 0 && j >= 0 && i < nx && j < ny && comp[at(i, j)] == c;
            };
            std::vector<char> seen((size_t)W * Hh, 0);
            std::vector<std::pair<int, int>> st;
            for (int u = 0; u < W; ++u)
                for (int v = 0; v < Hh; ++v)
                    if (u == 0 || v == 0 || u == W - 1 || v == Hh - 1) {
                        seen[(size_t)u * Hh + v] = 1;
                        st.push_back({u, v});
                    }
            const int du[4] = {-1, 1, 0, 0}, dv[4] = {0, 0, -1, 1};
            while (!st.empty()) {
                const int u = st.back().first, v = st.back().second;
                st.pop_back();
                for (int k = 0; k < 4; ++k) {
                    const int uu = u + du[k], vv = v + dv[k];
                    if (uu < 0 || vv < 0 || uu >= W || vv >= Hh) continue;
                    if (seen[(size_t)uu * Hh + vv] || own(uu, vv)) continue;
                    seen[(size_t)uu * Hh + vv] = 1;
                    st.push_back({uu, vv});
                }
            }
            int hu = -1, hv = -1;
            for (int v = 1; v < Hh - 1 && hu < 0; ++v)
                for (int u = 1; u < W - 1; ++u)
                    if (!seen[(size_t)u * Hh + v] && !own(u, v)) { hu = u; hv = v; break; }
            if (hu < 0) continue;
            int hi0 = hu, hi1 = hu, hj0 = hv, hj1 = hv;
            {
                std::vector<std::pair<int, int>> reg{{hu, hv}};
                std::vector<char> in((size_t)W * Hh, 0);
                in[(size_t)hu * Hh + hv] = 1;
                for (size_t qi = 0; qi < reg.size(); ++qi)
                    for (int k = 0; k < 4; ++k) {
                        const int uu = reg[qi].first + du[k], vv = reg[qi].second + dv[k];
                        if (uu < 1 || vv < 1 || uu >= W - 1 || vv >= Hh - 1) continue;
                        if (in[(size_t)uu * Hh + vv] || seen[(size_t)uu * Hh + vv] || own(uu, vv)) continue;
                        in[(size_t)uu * Hh + vv] = 1;
                        reg.push_back({uu, vv});
                        hi0 = std::min(hi0, uu); hi1 = std::max(hi1, uu);
                        hj0 = std::min(hj0, vv); hj1 = std::max(hj1, vv);
                    }
            }
            hi0 += b[0] - 1; hi1 += b[0] - 1; hj0 += b[1] - 1; hj1 += b[1] - 1;
            // Pieces of c left by a full cut along one grid line through the hole.
            auto pieces_after = [&](bool horiz, int L) {
                std::vector<char> mark((size_t)nx * ny, 0);
                int np = 0;
                std::vector<std::pair<int, int>> q2;
                for (int i = b[0]; i <= b[2]; ++i)
                    for (int j = b[1]; j <= b[3]; ++j) {
                        if (comp[at(i, j)] != c || mark[at(i, j)]) continue;
                        ++np;
                        mark[at(i, j)] = 1;
                        q2.push_back({i, j});
                        while (!q2.empty()) {
                            const int a = q2.back().first, e = q2.back().second;
                            q2.pop_back();
                            for (int k = 0; k < 4; ++k) {
                                const int u = a + du[k], v = e + dv[k];
                                if (u < 0 || v < 0 || u >= nx || v >= ny) continue;
                                if (comp[at(u, v)] != c || mark[at(u, v)]) continue;
                                if (k < 2) {
                                    const int bi = std::max(a, u);
                                    if (vcut[vcut_at(bi, e)] || (!horiz && bi == L)) continue;
                                } else {
                                    const int bj = std::max(e, v);
                                    if (cut[cut_at(a, bj)] || (horiz && bj == L)) continue;
                                }
                                mark[at(u, v)] = 1;
                                q2.push_back({u, v});
                            }
                        }
                    }
                return np;
            };
            bool best_h = true;
            int best_l = hj0, best_n = -1;
            for (int L = hj0; L <= hj1 + 1; ++L) {
                const int n = pieces_after(true, L);
                if (best_n < 0 || n < best_n) { best_n = n; best_h = true; best_l = L; }
            }
            for (int L = hi0; L <= hi1 + 1; ++L) {
                const int n = pieces_after(false, L);
                if (n < best_n) { best_n = n; best_h = false; best_l = L; }
            }
            if (best_h) {
                for (int i = b[0]; i <= b[2]; ++i)
                    if (best_l > 0 && best_l < ny && comp[at(i, best_l - 1)] == c && comp[at(i, best_l)] == c) cut[cut_at(i, best_l)] = 1;
            } else {
                for (int j = b[1]; j <= b[3]; ++j)
                    if (best_l > 0 && best_l < nx && comp[at(best_l - 1, j)] == c && comp[at(best_l, j)] == c) vcut[vcut_at(best_l, j)] = 1;
            }
            opened = true;
        }
        if (!opened) break;
    }
    label();
    std::vector<int> source((size_t)ncomp, -1), out_source;
    for (size_t k = 0; k < comp.size(); ++k)
        if (comp[k] >= 0) source[(size_t)comp[k]] = original_comp[k];
    std::vector<std::map<Pt, Pt>> nxt((size_t)ncomp);
    std::vector<char> bad((size_t)ncomp, 0);
    auto edge = [&](int c, const Pt& a, const Pt& e) {
        if (!nxt[(size_t)c].emplace(a, e).second) bad[(size_t)c] = 1;
    };
    for (int i = 0; i < nx; ++i)
        for (int j = 0; j < ny; ++j) {
            const int c = comp[at(i, j)];
            if (c < 0) continue;
            auto same = [&](int u, int v) { return u >= 0 && v >= 0 && u < nx && v < ny && comp[at(u, v)] == c; };
            const long x0 = xs[i], x1 = xs[i + 1], y0 = ys[j], y1 = ys[j + 1];
            if (!same(i, j - 1)) edge(c, Pt{{x0, y0}}, Pt{{x1, y0}});
            if (!same(i + 1, j)) edge(c, Pt{{x1, y0}}, Pt{{x1, y1}});
            if (!same(i, j + 1)) edge(c, Pt{{x1, y1}}, Pt{{x0, y1}});
            if (!same(i - 1, j)) edge(c, Pt{{x0, y1}}, Pt{{x0, y0}});
        }
    for (int c = 0; c < ncomp; ++c) {
        const auto& m = nxt[(size_t)c];
        std::vector<Pt> pts;
        if (!bad[(size_t)c] && !m.empty()) {
            const Pt start = m.begin()->first;
            Pt p = start;
            size_t steps = 0;
            do {
                pts.push_back(p);
                auto it = m.find(p);
                if (it == m.end()) { bad[(size_t)c] = 1; break; }
                p = it->second;
                ++steps;
            } while (!(p == start) && steps <= m.size());
            if (steps != m.size()) bad[(size_t)c] = 1;
        }
        if (bad[(size_t)c]) {
            for (int i = 0; i < nx; ++i) {
                int j = 0;
                while (j < ny) {
                    if (comp[at(i, j)] != c) { ++j; continue; }
                    int k = j;
                    while (k < ny && comp[at(i, k)] == c) ++k;
                    out.push_back({Pt{{xs[i], ys[j]}}, Pt{{xs[i + 1], ys[j]}}, Pt{{xs[i + 1], ys[k]}}, Pt{{xs[i], ys[k]}}});
                    out_source.push_back(source[(size_t)c]);
                    j = k;
                }
            }
            continue;
        }
        std::vector<Pt> s;
        const size_t n = pts.size();
        for (size_t k = 0; k < n; ++k) {
            const Pt& a = pts[(k + n - 1) % n];
            const Pt& q = pts[k];
            const Pt& d = pts[(k + 1) % n];
            const bool straight = (a[0] == q[0] && q[0] == d[0]) || (a[1] == q[1] && q[1] == d[1]);
            if (!straight) s.push_back(q);
        }
        out.push_back(s);
        out_source.push_back(source[(size_t)c]);
    }
    // Keep the existing fracture/outline construction. Only stitch pieces belonging
    // to one pre-cut component, at an oppositely traversed shared edge. Traversing
    // the two closed outlines at this point adds no area: shared edges cancel,
    // leaving a GDS keyhole boundary rather than separate touching boundaries.
    auto closed_at = [](const std::vector<Pt>& poly, const Pt& q) {
        std::vector<Pt> p = poly;
        auto it = std::find(p.begin(), p.end(), q);
        if (it == p.end()) {
            size_t i = 0;
            for (; i < p.size(); ++i) {
                const Pt a = p[i], b = p[(i + 1) % p.size()];
                if (((a[0] == b[0] && q[0] == a[0]) || (a[1] == b[1] && q[1] == a[1])) &&
                    q[0] >= std::min(a[0], b[0]) && q[0] <= std::max(a[0], b[0]) &&
                    q[1] >= std::min(a[1], b[1]) && q[1] <= std::max(a[1], b[1])) break;
            }
            if (i == p.size()) throw std::runtime_error("GDS hole bridge is not on its outline");
            it = p.insert(p.begin() + i + 1, q);
        }
        std::rotate(p.begin(), it, p.end());
        p.push_back(q);
        return p;
    };
    bool joined = true;
    while (joined) {
        joined = false;
        for (size_t a = 0; a < out.size() && !joined; ++a)
            for (size_t b = a + 1; b < out.size() && !joined; ++b) {
                if (out_source[a] != out_source[b]) continue;
                Pt q{};
                bool shared = false;
                for (size_t i = 0; i < out[a].size() && !shared; ++i)
                    for (size_t j = 0; j < out[b].size() && !shared; ++j) {
                        const Pt u = out[a][i], v = out[a][(i + 1) % out[a].size()];
                        const Pt w = out[b][j], z = out[b][(j + 1) % out[b].size()];
                        for (int axis = 0; axis < 2 && !shared; ++axis) {
                            const int other = 1 - axis;
                            const long lo = std::max(std::min(u[axis], v[axis]), std::min(w[axis], z[axis]));
                            const long hi = std::min(std::max(u[axis], v[axis]), std::max(w[axis], z[axis]));
                            if (u[other] == v[other] && w[other] == z[other] && u[other] == w[other] &&
                                lo < hi && (u[axis] < v[axis]) != (w[axis] < z[axis])) {
                                q[axis] = lo; q[other] = u[other]; shared = true;
                            }
                        }
                    }
                if (!shared) continue;
                auto first = closed_at(out[a], q), second = closed_at(out[b], q);
                first.insert(first.end(), second.begin() + 1, second.end() - 1);
                out[a] = std::move(first);
                out.erase(out.begin() + b);
                out_source.erase(out_source.begin() + b);
                joined = true;
            }
    }
    return out;
}

// Writes every layer as merged polygons, with zero-width bridges for holes.
void write_merged_boundaries(std::ofstream& f, const std::map<int, std::vector<Rect>>& rects,
                             double dbu_scale, int boundary_gds_layer) {
    for (const auto& kv : rects) {
        std::vector<IRect> rs;
        for (const Rect& r : kv.second) rs.push_back(to_irect(r));
        for (const auto& poly : merged_polygons(rs)) {
            if (poly.size() < 3) continue;
            record(f, GDS_BOUNDARY, {});
            { std::vector<uint8_t> d; put16(d, (uint16_t)kv.first); record(f, GDS_LAYER, d); }
            { std::vector<uint8_t> d; const uint16_t dt = (kv.first == boundary_gds_layer) ? 4 : 0; put16(d, dt); record(f, GDS_DATATYPE, d); }
            {
                std::vector<uint8_t> d;
                for (const Pt& p : poly) {
                    put32(d, (int32_t)std::lround((double)p[0] * dbu_scale));
                    put32(d, (int32_t)std::lround((double)p[1] * dbu_scale));
                }
                put32(d, (int32_t)std::lround((double)poly[0][0] * dbu_scale));
                put32(d, (int32_t)std::lround((double)poly[0][1] * dbu_scale));
                record(f, GDS_XY, d);
            }
            record(f, GDS_ENDEL, {});
        }
    }
}

// Fills of every hole in a rectangle set that is empty, narrower than space in one direction, and clear of keep_out.
std::vector<IRect> small_hole_fills(const std::vector<IRect>& in, long space, const std::vector<IRect>& keep_out) {
    std::vector<IRect> out;
    std::vector<IRect> rs;
    for (const IRect& r : in) if (r[0] < r[2] && r[1] < r[3]) rs.push_back(r);
    if (rs.empty()) return out;
    std::vector<long> xs, ys;
    for (const IRect& r : rs) { xs.push_back(r[0]); xs.push_back(r[2]); ys.push_back(r[1]); ys.push_back(r[3]); }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    const int nx = (int)xs.size() - 1, ny = (int)ys.size() - 1;
    auto at = [&](int i, int j) { return (size_t)i * (size_t)ny + (size_t)j; };
    std::vector<char> fill((size_t)nx * ny, 0);
    for (const IRect& r : rs) {
        const int i0 = (int)(std::lower_bound(xs.begin(), xs.end(), r[0]) - xs.begin());
        const int i1 = (int)(std::lower_bound(xs.begin(), xs.end(), r[2]) - xs.begin());
        const int j0 = (int)(std::lower_bound(ys.begin(), ys.end(), r[1]) - ys.begin());
        const int j1 = (int)(std::lower_bound(ys.begin(), ys.end(), r[3]) - ys.begin());
        for (int i = i0; i < i1; ++i)
            for (int j = j0; j < j1; ++j) fill[at(i, j)] = 1;
    }
    std::vector<int> comp((size_t)nx * ny, -1);
    int ncomp = 0;
    std::vector<std::pair<int, int>> st;
    for (int i = 0; i < nx; ++i)
        for (int j = 0; j < ny; ++j) {
            if (!fill[at(i, j)] || comp[at(i, j)] >= 0) continue;
            comp[at(i, j)] = ncomp;
            st.push_back({i, j});
            while (!st.empty()) {
                const int a = st.back().first, b = st.back().second;
                st.pop_back();
                const int da[4] = {-1, 1, 0, 0}, db[4] = {0, 0, -1, 1};
                for (int k = 0; k < 4; ++k) {
                    const int u = a + da[k], v = b + db[k];
                    if (u < 0 || v < 0 || u >= nx || v >= ny) continue;
                    if (!fill[at(u, v)] || comp[at(u, v)] >= 0) continue;
                    comp[at(u, v)] = ncomp;
                    st.push_back({u, v});
                }
            }
            ++ncomp;
        }
    std::vector<std::array<int, 4>> bb((size_t)ncomp, std::array<int, 4>{{nx, ny, -1, -1}});
    std::vector<long> cnt((size_t)ncomp, 0);
    for (int i = 0; i < nx; ++i)
        for (int j = 0; j < ny; ++j) {
            const int c = comp[at(i, j)];
            if (c < 0) continue;
            auto& b = bb[(size_t)c];
            b[0] = std::min(b[0], i); b[1] = std::min(b[1], j);
            b[2] = std::max(b[2], i); b[3] = std::max(b[3], j);
            ++cnt[(size_t)c];
        }
    const int du[4] = {-1, 1, 0, 0}, dv[4] = {0, 0, -1, 1};
    for (int c = 0; c < ncomp; ++c) {
        const std::array<int, 4> b = bb[(size_t)c];
        if ((long)(b[2] - b[0] + 1) * (long)(b[3] - b[1] + 1) == cnt[(size_t)c]) continue;
        const int W = b[2] - b[0] + 3, Hh = b[3] - b[1] + 3;
        auto own = [&](int u, int v) {
            const int i = b[0] - 1 + u, j = b[1] - 1 + v;
            return i >= 0 && j >= 0 && i < nx && j < ny && comp[at(i, j)] == c;
        };
        std::vector<char> seen((size_t)W * Hh, 0);
        std::vector<std::pair<int, int>> q;
        for (int u = 0; u < W; ++u)
            for (int v = 0; v < Hh; ++v)
                if (u == 0 || v == 0 || u == W - 1 || v == Hh - 1) { seen[(size_t)u * Hh + v] = 1; q.push_back({u, v}); }
        while (!q.empty()) {
            const int u = q.back().first, v = q.back().second;
            q.pop_back();
            for (int k = 0; k < 4; ++k) {
                const int uu = u + du[k], vv = v + dv[k];
                if (uu < 0 || vv < 0 || uu >= W || vv >= Hh) continue;
                if (seen[(size_t)uu * Hh + vv] || own(uu, vv)) continue;
                seen[(size_t)uu * Hh + vv] = 1;
                q.push_back({uu, vv});
            }
        }
        for (int u0 = 1; u0 < W - 1; ++u0)
            for (int v0 = 1; v0 < Hh - 1; ++v0) {
                if (seen[(size_t)u0 * Hh + v0] || own(u0, v0)) continue;
                std::vector<std::pair<int, int>> region{{u0, v0}};
                seen[(size_t)u0 * Hh + v0] = 1;
                for (size_t qi = 0; qi < region.size(); ++qi)
                    for (int k = 0; k < 4; ++k) {
                        const int uu = region[qi].first + du[k], vv = region[qi].second + dv[k];
                        if (uu < 0 || vv < 0 || uu >= W || vv >= Hh) continue;
                        if (seen[(size_t)uu * Hh + vv] || own(uu, vv)) continue;
                        seen[(size_t)uu * Hh + vv] = 1;
                        region.push_back({uu, vv});
                    }
                bool empty = true;
                long x0 = 0, y0 = 0, x1 = 0, y1 = 0;
                bool first = true;
                for (const auto& cell : region) {
                    const int i = b[0] - 1 + cell.first, j = b[1] - 1 + cell.second;
                    if (fill[at(i, j)]) empty = false;
                    if (first) { x0 = xs[i]; y0 = ys[j]; x1 = xs[i + 1]; y1 = ys[j + 1]; first = false; }
                    x0 = std::min(x0, xs[i]); y0 = std::min(y0, ys[j]);
                    x1 = std::max(x1, xs[i + 1]); y1 = std::max(y1, ys[j + 1]);
                }
                if (!empty || (x1 - x0 >= space && y1 - y0 >= space)) continue;
                const IRect hb{{x0, y0, x1, y1}};
                bool blocked = false;
                for (const IRect& k : keep_out) if (ir_overlap(k, hb)) blocked = true;
                if (blocked) continue;
                for (const auto& cell : region) {
                    const int i = b[0] - 1 + cell.first, j = b[1] - 1 + cell.second;
                    out.push_back({xs[i], ys[j], xs[i + 1], ys[j + 1]});
                }
            }
    }
    return out;
}

// Contact enclosure, extra contacts and pin shapes from the IHP review, drawn after the solve without touching the routing.
void apply_ihp_feedback(const Config& cfg, const RoutingResult& res, const std::vector<Net>& nets,
                        const NetOrders& no, std::map<int, std::vector<Rect>>& rects,
                        std::vector<GdsLabel>& labels, std::vector<Rect>& m1_pin_rects) {
    const long ch = cfg.cell_height;
    const int gl_m1 = gds_layer(cfg, "M1"), gl_cont = gds_layer(cfg, "Cont");
    const int gl_act = gds_layer(cfg, "Active"), gl_gate = gds_layer(cfg, "Gate");
    const long hw = cfg.width("Cont") / 2;
    const long enc = cfg.rules.at("enclosure").at("Cont").at("M1").get<long>();
    const long m1_space = cfg.rules.at("spacing").at("S2S").at("M1").at("M1").get<long>();
    // Deck values the config does not carry: Cnt.b, Cnt.c, Cnt.d, Cnt.e, Cnt.f, Gat.b, Gat.d.
    const long cont_space = cfg.opt_long("deck_cont_space", 180);
    const long act_enc = cfg.opt_long("active_contact_enclosure", 70);
    const long gate_enc = cfg.opt_long("deck_gate_cont_enclosure", 70);
    const long gcont_act = cfg.opt_long("deck_gate_cont_active_space", 140);
    const long cont_gate = cfg.opt_long("deck_active_cont_gate_space", 110);
    const long gate_space = cfg.opt_long("deck_gate_space", 180);
    const long gate_act = cfg.opt_long("deck_gate_active_space", 70);
    const long pitch = 2 * hw + cont_space;
    const long half = pitch / 2;
    const long gp = (long)cfg.pitch.at("Gate");

    Pieces m1, poly;
    std::vector<IRect> act, conts;
    for (const Rect& r : rects[gl_m1]) m1.r.push_back(to_irect(r));
    for (const Rect& r : rects[gl_gate]) poly.r.push_back(to_irect(r));
    for (const Rect& r : rects[gl_act]) act.push_back(to_irect(r));
    for (const Rect& r : rects[gl_cont]) conts.push_back(to_irect(r));
    m1.relabel();
    poly.relabel();

    auto cx_of = [](const IRect& r) { return (r[0] + r[2]) / 2; };
    auto cy_of = [](const IRect& r) { return (r[1] + r[3]) / 2; };
    auto box_at = [&](long x, long y) { return IRect{{x - hw, y - hw, x + hw, y + hw}}; };
    auto on_rail = [&](const IRect& c) { const long y = (c[1] + c[3]) / 2; return y == 0 || y == ch; };
    auto over = [](const IRect& c, const std::vector<IRect>& rs) {
        for (const IRect& r : rs) if (ir_overlap(c, r)) return true;
        return false;
    };
    auto bar_of = [&](long x, long ya, long yb) {
        return IRect{{x - hw - enc, std::min(ya, yb) - hw - enc, x + hw + enc, std::max(ya, yb) + hw + enc}};
    };
    const std::vector<IRect> original = conts;
    auto m1_try = [&](const IRect& c, int own) {
        std::vector<IRect> fills;
        if (!m1.clear_fill(c, own, m1_space, fills)) return false;
        m1.add(c, own);
        for (const IRect& f : fills) m1.add(f, own);
        return true;
    };

    // Metal1 grown to the enclosure on all four sides of every contact where the space allows.
    for (const IRect& c : original) {
        const int own = m1.at(cx_of(c), cy_of(c));
        if (own < 0) continue;
        const IRect pad = ir_grow(c, enc);
        if (ir_covered(pad, m1.r) || m1_try(pad, own)) continue;
        for (int side = 0; side < 4; ++side) {
            for (long e = enc; e > 0; e -= 5) {
                IRect s = c;
                if (side == 0) { s[0] = c[0] - e; s[2] = c[0]; }
                else if (side == 1) { s[0] = c[2]; s[2] = c[2] + e; }
                else if (side == 2) { s[1] = c[1] - e; s[3] = c[1]; }
                else { s[1] = c[3]; s[3] = c[3] + e; }
                if (ir_covered(s, m1.r) || m1_try(s, own)) break;
            }
        }
    }

    auto sd_ok = [&](long x, long y, const IRect* skip) {
        const IRect b = box_at(x, y);
        if (!ir_covered(ir_grow(b, act_enc), act)) return false;
        for (const IRect& g : poly.r) if (ir_gap(b, g) < (double)cont_gate) return false;
        for (const IRect& c : conts) {
            if (skip && c == *skip) continue;
            if (ir_gap(b, c) < (double)cont_space) return false;
        }
        return true;
    };
    auto same_diff = [&](long x, long ya, long yb) {
        const IRect s{{x - hw, std::min(ya, yb) - hw, x + hw, std::max(ya, yb) + hw}};
        return ir_covered(s, act) && !over(s, poly.r);
    };

    auto sd_pair = [&](const IRect& c0, int own) {
        for (const IRect& c : conts) {
            if (c == c0 || on_rail(c) || !over(c, act) || ir_gap(c0, c) < (double)cont_space) continue;
            if (m1.at(cx_of(c), cy_of(c)) != own) continue;
            const IRect span = ir_hull(c0, c);
            if (ir_covered(span, act) && !over(span, poly.r)) return true;
        }
        return false;
    };

    // Source and drain contacts become a pair on the same diffusion, then more while they fit.
    for (const IRect& c0 : original) {
        if (on_rail(c0) || !over(c0, act)) continue;
        const long x = cx_of(c0), y = cy_of(c0);
        const int own = m1.at(x, y);
        if (own < 0 || sd_pair(c0, own)) continue;
        bool placed = false;
        long lo = y, hi = y;
        for (long k = 0; k <= half && !placed; k += 5) {
            for (int sg = 1; sg >= -1 && !placed; sg -= 2) {
                if (k == 0 && sg < 0) continue;
                const long s = sg * k;
                const long y0 = y + s - half, y1 = y + s + half;
                if (!sd_ok(x, y0, &c0) || !sd_ok(x, y1, &c0) || !same_diff(x, y0, y1)) continue;
                const IRect bar = bar_of(x, y0, y1);
                if (!m1_try(bar, own)) continue;
                auto it = std::find(conts.begin(), conts.end(), c0);
                if (it != conts.end()) conts.erase(it);
                conts.push_back(box_at(x, y0));
                conts.push_back(box_at(x, y1));
                lo = y0;
                hi = y1;
                placed = true;
            }
        }
        if (!placed) {
            // Try the same pair horizontally only after the vertical candidates fail.
            for (long k = 0; k <= half && !placed; k += 5) {
                for (int sg = 1; sg >= -1 && !placed; sg -= 2) {
                    if (k == 0 && sg < 0) continue;
                    const long x0 = x + sg * k - half, x1 = x + sg * k + half;
                    if (!sd_ok(x0, y, &c0) || !sd_ok(x1, y, &c0)) continue;
                    const IRect span = ir_hull(box_at(x0, y), box_at(x1, y));
                    if (!ir_covered(span, act) || over(span, poly.r)) continue;
                    if (!m1_try(ir_grow(span, enc), own)) continue;
                    auto it = std::find(conts.begin(), conts.end(), c0);
                    if (it != conts.end()) conts.erase(it);
                    conts.push_back(box_at(x0, y));
                    conts.push_back(box_at(x1, y));
                    placed = true;
                }
            }
            continue;
        }
        for (int dir = 1; dir >= -1; dir -= 2) {
            for (int guard = 0; guard < 8; ++guard) {
                const long yn = (dir > 0) ? hi + pitch : lo - pitch;
                if (!sd_ok(x, yn, nullptr)) break;
                const long nlo = std::min(lo, yn), nhi = std::max(hi, yn);
                if (!same_diff(x, nlo, nhi)) break;
                const IRect bar = bar_of(x, nlo, nhi);
                if (!m1_try(bar, own)) break;
                conts.push_back(box_at(x, yn));
                lo = nlo;
                hi = nhi;
            }
        }
    }

    // A source tied to the rail through its Activ leg gets contacts in its diffusion, strapped to the rail on Metal1.
    for (const IRect& c0 : original) {
        if (!on_rail(c0)) continue;
        const long x = cx_of(c0), yr = cy_of(c0);
        const long dir = (yr == 0) ? 1 : -1;
        long ya = 0;
        bool found = false;
        for (long d = pitch; d <= ch / 2; d += 5) {
            const long yy = yr + dir * d;
            if (!same_diff(x, yr, yy)) break;
            if (sd_ok(x, yy, nullptr)) { ya = yy; found = true; break; }
        }
        if (!found) continue;
        const int own = m1.at(x, yr);
        if (own < 0) continue;
        for (int guard = 0; guard < 8; ++guard) {
            const long yy = ya + dir * pitch * guard;
            if (!sd_ok(x, yy, nullptr) || !same_diff(x, yr, yy)) break;
            const IRect strap = (dir > 0) ? IRect{{x - hw - enc, yr, x + hw + enc, yy + hw + enc}}
                                          : IRect{{x - hw - enc, yy - hw - enc, x + hw + enc, yr}};
            if (!m1_try(strap, own)) break;
            conts.push_back(box_at(x, yy));
        }
    }

    auto gate_cont_ok = [&](long x, long y, const IRect* skip) {
        const IRect b = box_at(x, y);
        for (const IRect& a : act) if (ir_gap(b, a) < (double)gcont_act) return false;
        for (const IRect& c : conts) {
            if (skip && c == *skip) continue;
            if (ir_gap(b, c) < (double)cont_space) return false;
        }
        return true;
    };
    auto poly_ok = [&](const IRect& p, int own) {
        if (!poly.clear(p, own, gate_space)) return false;
        for (const IRect& a : act) if (ir_gap(p, a) < (double)gate_act) return false;
        return true;
    };

    auto gate_pair = [&](const IRect& c0, int own_m, int own_p) {
        for (const IRect& c : conts) {
            if (c == c0 || on_rail(c) || over(c, act) || ir_gap(c0, c) < (double)cont_space) continue;
            if (m1.at(cx_of(c), cy_of(c)) == own_m && poly.at(cx_of(c), cy_of(c)) == own_p) return true;
        }
        return false;
    };

    // A gate contact gets a second one on the next finger of the same gate or beside it on its own poly.
    for (const IRect& c0 : original) {
        if (on_rail(c0) || over(c0, act) || !over(c0, poly.r)) continue;
        const long x = cx_of(c0), y = cy_of(c0);
        const int own_m = m1.at(x, y), own_p = poly.at(x, y);
        if (own_m < 0 || own_p < 0 || gate_pair(c0, own_m, own_p)) continue;
        const IRect pad0 = ir_grow(c0, gate_enc);
        bool done = false;
        const long keep[6][2] = {{gp, 0}, {-gp, 0}, {0, pitch}, {0, -pitch}, {pitch, 0}, {-pitch, 0}};
        for (int t = 0; t < 6 && !done; ++t) {
            const long xn = x + keep[t][0], yn = y + keep[t][1];
            if (!gate_cont_ok(xn, yn, nullptr)) continue;
            IRect pad = ir_grow(box_at(xn, yn), gate_enc);
            if (ir_gap(pad, pad0) < (double)gate_space || !poly.touches(pad, own_p)) pad = ir_hull(pad, pad0);
            if (!poly_ok(pad, own_p)) continue;
            const IRect bar = ir_hull(ir_grow(c0, enc), ir_grow(box_at(xn, yn), enc));
            if (!m1_try(bar, own_m)) continue;
            poly.add(pad, own_p);
            conts.push_back(box_at(xn, yn));
            done = true;
        }
        for (int axis = 1; axis >= 0 && !done; --axis) {
            for (long k = 0; k < half && !done; k += 5) {
                for (int sg = 1; sg >= -1 && !done; sg -= 2) {
                    if (k == 0 && sg < 0) continue;
                    const long s = sg * k;
                    const long x0 = axis ? x : x + s - half, x1 = axis ? x : x + s + half;
                    const long y0 = axis ? y + s - half : y, y1 = axis ? y + s + half : y;
                    if (!gate_cont_ok(x0, y0, &c0) || !gate_cont_ok(x1, y1, &c0)) continue;
                    const IRect span = ir_hull(box_at(x0, y0), box_at(x1, y1));
                    const IRect pad = ir_grow(span, gate_enc);
                    if (!poly_ok(pad, own_p)) continue;
                    if (!m1_try(ir_grow(span, enc), own_m)) continue;
                    poly.add(pad, own_p);
                    auto it = std::find(conts.begin(), conts.end(), c0);
                    if (it != conts.end()) conts.erase(it);
                    conts.push_back(box_at(x0, y0));
                    conts.push_back(box_at(x1, y1));
                    done = true;
                }
            }
        }
    }

    for (const IRect& f : small_hole_fills(m1.r, m1_space, std::vector<IRect>())) m1.add(f, -1);
    for (const IRect& f : small_hole_fills(poly.r, gate_space, act)) poly.add(f, -1);
    rects[gl_m1].clear();
    for (const IRect& r : m1.r) rects[gl_m1].push_back(to_rect(r));
    rects[gl_gate].clear();
    for (const IRect& r : poly.r) rects[gl_gate].push_back(to_rect(r));

    const long site = cfg.opt_long("site_width");
    const long np = ((long)no.p_net.size() + 1) / 2;
    const double gate_span = (double)np * cfg.pitch.at("Gate");
    const double max_x = (site > 0) ? std::ceil(gate_span / (double)site) * (double)site : gate_span;
    const long shift = (site > 0 && max_x > gate_span) ? std::lround((max_x - gate_span) / 2.0) : 0;

    // Rail tap contacts sit on every site centre as in the PDK cells, so the rows either side of a shared rail line them up.
    if (site > 0) {
        conts.erase(std::remove_if(conts.begin(), conts.end(), on_rail), conts.end());
        const std::vector<IRect> inner = conts;
        for (const long yr : {0L, ch}) {
            for (long x = site / 2; x + hw <= std::lround(max_x); x += site) {
                const IRect b = box_at(x, yr);
                bool ok = ir_covered(ir_grow(b, act_enc), act);
                for (const IRect& c : inner) if (ok && ir_gap(b, c) < (double)cont_space) ok = false;
                for (const IRect& g : poly.r) if (ok && ir_gap(b, g) < (double)cont_gate) ok = false;
                if (ok) conts.push_back(b);
            }
        }
    }
    rects[gl_cont].clear();
    for (const IRect& r : conts) rects[gl_cont].push_back(to_rect(r));

    // VDD and VSS pins cover the whole rail, the same extent the rails are drawn with.
    m1.relabel();
    m1_pin_rects.clear();
    const double pwm = (double)cfg.power_width("M1");
    m1_pin_rects.push_back({0.0, -pwm / 2.0, max_x, pwm / 2.0});
    m1_pin_rects.push_back({0.0, (double)ch - pwm / 2.0, max_x, (double)ch + pwm / 2.0});

    // A signal pin is the largest rectangle of the net's Metal1 that holds a routing grid point.
    const long m1_z = cfg.routing_layer_index("M1");
    std::map<std::array<long, 3>, std::vector<std::array<long, 3>>> adj;
    for (const auto& e : res.metals) {
        adj[e.first].push_back(e.second);
        adj[e.second].push_back(e.first);
    }
    const long gx = cfg.opt_long("pin_grid_x", 480), gy = cfg.opt_long("pin_grid_y", 420);
    const long cap = cfg.width("M1") / 2;
    const size_t signal_count = std::count_if(nets.begin(), nets.end(),
        [](const Net& net) { return net.is_ext_pin && !net.is_power; });
    if (signal_count > 0 && (gx <= 0 || gy <= 0))
        throw std::runtime_error("IHP signal pins require a positive routing grid");
    for (const Net& net : nets) {
        if (!net.is_ext_pin || net.is_power) continue;
        GdsLabel* lab = nullptr;
        for (GdsLabel& l : labels)
            if (l.text == net.name && l.layer == gl_m1) { lab = &l; break; }
        if (lab == nullptr) throw std::runtime_error("No Metal1 label for required port " + net.name);
        const long lx = std::lround(lab->x), ly = std::lround(lab->y);
        std::set<int> pieces;
        const int p0 = m1.at(lx, ly);
        if (p0 >= 0) pieces.insert(p0);
        const std::array<long, 3> start{{lx - shift, ly, m1_z}};
        if (adj.count(start)) {
            std::set<std::array<long, 3>> seen{start};
            std::vector<std::array<long, 3>> q{start};
            for (size_t qi = 0; qi < q.size(); ++qi) {
                const std::array<long, 3> cur = q[qi];
                if (cur[2] == m1_z) {
                    const int p = m1.at(cur[0] + shift, cur[1]);
                    if (p >= 0) pieces.insert(p);
                }
                for (const auto& n : adj[cur])
                    if (seen.insert(n).second) q.push_back(n);
            }
        }
        PinPick best;
        for (int p : pieces) {
            std::vector<IRect> rs;
            for (size_t i = 0; i < m1.r.size(); ++i)
                if (m1.comp[i] == p) rs.push_back(m1.r[i]);
            const PinPick pk = best_rect(rs, gx, gy, false, 0, 0, cap);
            if (pk.found && (pk.margin > best.margin || (pk.margin == best.margin && pk.area > best.area))) best = pk;
        }
        if (!best.found) {
            // No Metal1 of the net holds a grid point, so one piece of it grows by a rectangle that does.
            const PinExt ext = grid_extension(m1, pieces, gx, gy, cap, m1_space, cfg.width("M1"),
                                              cfg.opt_long("deck_m1_wide_width", 300), cfg.opt_long("deck_m1_wide_space", 220),
                                              cfg.opt_long("pin_ext_reach", 1000), cfg.opt_long("pin_ext_edge_reach", 800),
                                              std::lround(max_x), ch);
            if (ext.piece >= 0) {
                std::vector<IRect> rs;
                for (size_t i = 0; i < m1.r.size(); ++i)
                    if (m1.comp[i] == ext.piece) rs.push_back(m1.r[i]);
                rs.push_back(ext.r);
                const PinPick pk = best_rect(rs, gx, gy, false, 0, 0, cap);
                if (pk.found && pk.margin > 0) {
                    m1.add(ext.r, ext.piece);
                    rects[gl_m1].push_back(to_rect(ext.r));
                    best = pk;
                }
            }
        }
        if (!best.found) {
            // No single rectangle does, so the piece grows by two, the second holding the grid point.
            const PinExt2 ext2 = grid_extension2(m1, pieces, gx, gy, cap, m1_space, cfg.width("M1"),
                                                 cfg.opt_long("deck_m1_wide_width", 300), cfg.opt_long("deck_m1_wide_space", 220),
                                                 cfg.opt_long("pin_ext_reach", 1000), cfg.opt_long("pin_ext_edge_reach", 800),
                                                 std::lround(max_x), ch);
            if (ext2.piece >= 0) {
                std::vector<IRect> rs;
                for (size_t i = 0; i < m1.r.size(); ++i)
                    if (m1.comp[i] == ext2.piece) rs.push_back(m1.r[i]);
                rs.push_back(ext2.r1);
                rs.push_back(ext2.r2);
                const PinPick pk = best_rect(rs, gx, gy, false, 0, 0, cap);
                if (pk.found && pk.margin > 0) {
                    m1.add(ext2.r1, ext2.piece);
                    m1.add(ext2.r2, ext2.piece);
                    rects[gl_m1].push_back(to_rect(ext2.r1));
                    rects[gl_m1].push_back(to_rect(ext2.r2));
                    best = pk;
                }
            }
        }
        if (!best.found)
            throw std::runtime_error("No Metal1 of required port " + net.name + " holds a routing grid point");
        m1_pin_rects.push_back(to_rect(best.r));
        lab->x = (double)(((best.r[0] + best.r[2]) / 2) / 5 * 5);
        lab->y = (double)(((best.r[1] + best.r[3]) / 2) / 5 * 5);
    }
    if (m1_pin_rects.size() != signal_count + 2)
        throw std::runtime_error("IHP required signal pin count mismatch");
    for (size_t i = 2; i < m1_pin_rects.size(); ++i) {
        const IRect pin = to_irect(m1_pin_rects[i]);
        if (pin[0] >= pin[2] || pin[1] >= pin[3] || !ir_covered(pin, m1.r) ||
            first_mult(pin[0], gx) > pin[2] || first_mult(pin[1], gy) > pin[3])
            throw std::runtime_error("IHP signal pin coverage or routing grid invariant failed");
    }

    // Enclosure counts final contacts, pair counts original non-rail sites, reasons count unmet checks.
    m1.relabel();
    poly.relabel();
    size_t enc_unmet = 0, sd_unmet = 0, gate_unmet = 0, no_m1 = 0;
    for (const IRect& c : conts) {
        if (ir_covered(ir_grow(c, enc), m1.r)) continue;
        ++enc_unmet;
        if (m1.at(cx_of(c), cy_of(c)) < 0) ++no_m1;
    }
    for (const IRect& c0 : original) {
        if (on_rail(c0)) continue;
        const int own_m = m1.at(cx_of(c0), cy_of(c0));
        size_t count = 0;
        const bool sd = over(c0, act);
        const int own_p = poly.at(cx_of(c0), cy_of(c0));
        if (!sd && own_p < 0) continue;
        for (const IRect& c : conts) {
            if (on_rail(c) || own_m < 0 || m1.at(cx_of(c), cy_of(c)) != own_m) continue;
            const IRect span = ir_hull(c0, c);
            if (sd ? (over(c, act) && ir_covered(span, act) && !over(span, poly.r))
                   : (!over(c, act) && poly.at(cx_of(c), cy_of(c)) == own_p)) ++count;
        }
        if (count < 2) {
            if (sd) ++sd_unmet;
            else ++gate_unmet;
            if (own_m < 0) ++no_m1;
        }
    }
    if (enc_unmet || sd_unmet || gate_unmet)
        std::cerr << "IHP optional feedback: m1_enclosure_unmet=" << enc_unmet
                  << " sd_unmet=" << sd_unmet << " gate_unmet=" << gate_unmet
                  << " unmet_checks(missing_metal1=" << no_m1
                  << ",no_candidate_in_search=" << enc_unmet + sd_unmet + gate_unmet - no_m1 << ")" << std::endl;
}

}

void write_routing_gds(const std::string& path, const std::string& cell_name, const Config& cfg,
                       const RoutingResult& res, const PreLayout& pre, const NetOrders& no,
                       const std::vector<Net>& nets) {
    const long ch = cfg.cell_height;
    const long pw_last = cfg.power_width(cfg.power_layer.back());
    const double clamp_lo = -pw_last / 2.0, clamp_hi = ch + pw_last / 2.0;

    std::map<int, std::vector<Rect>> rects;
    std::vector<GdsLabel> labels;

    LisdExtents lisd_fixed = compute_lisd_fixed_extents(cfg, no, nets);

    emit_metals(cfg, pre, res.metals, rects, clamp_lo, clamp_hi, &lisd_fixed);
    emit_via_enclosure_rects(cfg, res, rects, clamp_lo, clamp_hi);

    const auto& layers = cfg.routing_layers;
    std::set<std::string> port_names;
    for (const Net& n : nets) if (n.is_ext_pin) port_names.insert(n.name);
    std::map<std::string, ExtPin> ext_by_net;
    for (const auto& p : res.ext_pins)
        if (port_names.count(p.net)) ext_by_net[p.net] = p;
    for (const auto& kv : ext_by_net) {
        const auto& p = kv.second;
        labels.push_back({p.net, (double)p.x, (double)p.y, gds_layer(cfg, layers[p.z]), label_texttype(cfg, layers[p.z])});
    }

    // Metal1.pin (8/2): the reference library duplicates the M1 shape at
    // each exposed pin location onto a separate pin-purpose datatype, on
    // top of the M1.drawing (8/0) copy build_device_geometry()/emit_metals()
    // already produce. Every wishlist pin here lands on M1 (confirmed:
    // sg13g2_nand2_1's own pins are all on 8/2, none on 1/2 or 5/2), so this
    // only needs to run for M1.
    // Metal1.pin (8/2): the reference library duplicates the M1 shape at
    // each net's exposed pin location onto a separate pin-purpose datatype,
    // on top of the M1.drawing (8/0) copy build_device_geometry()/emit_metals()
    // already produce -- confirmed against sg13g2_nand2_1, which has exactly
    // one 8/2 shape per net (A, B, Y, VDD, VSS: 5 nets, 5 shapes), power nets
    // included.
    //
    // This can't be driven by res.ext_pins: the SAT model's EXT_PIN_* boolean
    // vars come out empty for this router mode (confirmed by instrumentation --
    // res.ext_pins.size()==0 for a cell whose GDS clearly has M1 routing), so
    // ext_by_net is always empty here too. Walk `nets` instead, the same
    // source emit_external_label_fallback() below already uses for its text
    // labels, but -- unlike that fallback -- without skipping power nets,
    // since the reference convention marks VDD/VSS pins the same as signal
    // pins. Take the first point on M1 found across all of a net's pins.
    emit_external_label_fallback(cfg, nets, res, labels, site_pad_shift(cfg, no));

    build_device_geometry(cfg, no, nets, pre, rects, labels, clamp_lo, clamp_hi, lisd_fixed);

    // A pin shape marks a port, so it goes where the net reaches Metal1. Its
    // own pins are device terminals on Active and Gate, never on Metal1, so
    // the label just placed for this net is the Metal1 point to use. Power
    // nets are marked too: the reference library gives VDD and VSS a pin
    // shape like any other port.
    std::vector<Rect> m1_pin_rects;
    const int m1_gl_pin = gds_layer(cfg, "M1");
    std::set<std::string> pinned;
    auto mark = [&](const std::string& name) {
        if (name.empty() || name == DUMMY_NET || pinned.count(name)) return;
        for (const GdsLabel& lab : labels) {
            if (lab.text != name || lab.layer != m1_gl_pin) continue;
            m1_pin_rects.push_back(get_square(cfg, (long)lab.x, (long)lab.y,
                                              (long)lab.x, (long)lab.y, "M1", pre));
            pinned.insert(name);
            return;
        }
    };
    for (const Net& net : nets)
        if (net.is_ext_pin) mark(net.name);
    // The rails carry their own labels from emit_power_rails(), so the two
    // power ports are marked from those rather than from the fallback, which
    // leaves power alone.
    mark(cfg.power_net);
    mark(cfg.ground_net);


    {
        const long site = cfg.opt_long("site_width");
        const long np = ((long)no.p_net.size() + 1) / 2;
        const double gate_span = (double)np * cfg.pitch.at("Gate");
        if (site > 0) {
            const double max_x = std::ceil(gate_span / (double)site) * (double)site;
            const double shift = site_pad_shift(cfg, no);
            if (shift > 0.0) {
                auto move = [&](Rect& r) {
                    if (r[0] <= 0.5 && r[2] >= max_x - 0.5) return;
                    r[0] += shift;
                    r[2] += shift;
                };
                for (auto& kv : rects)
                    for (Rect& r : kv.second) move(r);
                for (Rect& r : m1_pin_rects) move(r);
                for (GdsLabel& l : labels) l.x += shift;
            }
        }
    }
    for (auto& kv : rects) kv.second = union_rects(kv.second);

    // M1.d, minimum metal area, repaired once on the merged shapes. Only a few
    // groups fall short, and growing every pad in the emitters instead regressed
    // cells that were already clear. For each group under the limit try the
    // compact candidates in turn and keep the first whose clearance to every
    // other group still meets the M1 spacing. That clearance is measured here,
    // not assumed: reaching the area by growing one axis measured 155 against a
    // 180 rule at six of ten sites.
    {
        // M1.d is 0.09 and M2.d is 0.144, so the target and the spacing are
        // read per layer rather than hardcoded for M1.
        for (const char* lname : {"M1", "M2"}) {
        const long m1_area = (std::string(lname) == "M1")
            ? cfg.opt_long("m1_min_area", 0)
            : cfg.opt_long("m2_min_area", 0);
        if (m1_area > 0) {
            const long m1_space = cfg.rules.at("spacing").at("S2S").at(lname).at(lname).get<long>();
            auto& rs = rects[gds_layer(cfg, lname)];
            const size_t n = rs.size();
            auto touches = [](const Rect& a, const Rect& b) {
                return a[0] <= b[2] && b[0] <= a[2] && a[1] <= b[3] && b[1] <= a[3];
            };
            auto gap = [](const Rect& a, const Rect& b) {
                const double dx = std::max(0.0, std::max(a[0] - b[2], b[0] - a[2]));
                const double dy = std::max(0.0, std::max(a[1] - b[3], b[1] - a[3]));
                return std::sqrt(dx * dx + dy * dy);
            };
            std::vector<int> comp(n, -1);
            int ncomp = 0;
            for (size_t i = 0; i < n; ++i) {
                if (comp[i] >= 0) continue;
                comp[i] = ncomp;
                std::vector<size_t> stack{i};
                while (!stack.empty()) {
                    const size_t c = stack.back();
                    stack.pop_back();
                    for (size_t j = 0; j < n; ++j)
                        if (comp[j] < 0 && touches(rs[c], rs[j])) { comp[j] = ncomp; stack.push_back(j); }
                }
                ++ncomp;
            }
            const double kGridNm = 5.0;  // manufacturing grid the deck checks
            const double side = std::ceil(std::sqrt((double)m1_area) / 5.0) * 5.0;
            for (int c = 0; c < ncomp; ++c) {
                std::vector<size_t> mem;
                double area = 0.0;
                for (size_t i = 0; i < n; ++i)
                    if (comp[i] == c) { mem.push_back(i); area += (rs[i][2] - rs[i][0]) * (rs[i][3] - rs[i][1]); }
                if (area >= (double)m1_area) continue;
                // The group's own bounding box, which for a multi-rect group is
                // a superset of its metal and therefore always legal to draw.
                Rect r{rs[mem[0]][0], rs[mem[0]][1], rs[mem[0]][2], rs[mem[0]][3]};
                for (size_t i : mem) {
                    r[0] = std::min(r[0], rs[i][0]); r[1] = std::min(r[1], rs[i][1]);
                    r[2] = std::max(r[2], rs[i][2]); r[3] = std::max(r[3], rs[i][3]);
                }
                const double cx = (r[0] + r[2]) / 2.0, cy = (r[1] + r[3]) / 2.0;
                const double w = r[2] - r[0], h = r[3] - r[1];
                const double ny = std::ceil((double)m1_area / w / 5.0) * 5.0;
                const double nx = std::ceil((double)m1_area / h / 5.0) * 5.0;
                // Centred first, then one sided, so a group hemmed in on one
                // side can still reach the area by growing away from it.
                const Rect cands[7] = {
                    {cx - side / 2.0, cy - side / 2.0, cx + side / 2.0, cy + side / 2.0},
                    {r[0], cy - ny / 2.0, r[2], cy + ny / 2.0},
                    {cx - nx / 2.0, r[1], cx + nx / 2.0, r[3]},
                    {r[0], r[3] - ny, r[2], r[3]},
                    {r[0], r[1], r[2], r[1] + ny},
                    {r[2] - nx, r[1], r[2], r[3]},
                    {r[0], r[1], r[0] + nx, r[3]},
                };
                // A centred candidate halves its dimension, so snapping the dimension
                // alone to the 5nm grid still lands an odd multiple's edges on 2.5nm
                // (395/2 = 197.5, which the deck reports as metal1_drw_Offgrid).
                // Snap the rectangle itself, outward: that only grows the area and
                // still covers the group bbox, and the clearance test below runs on
                // the snapped shape.
                for (Rect cand : cands) {
                    cand[0] = std::floor(cand[0] / kGridNm) * kGridNm;
                    cand[1] = std::floor(cand[1] / kGridNm) * kGridNm;
                    cand[2] = std::ceil(cand[2] / kGridNm) * kGridNm;
                    cand[3] = std::ceil(cand[3] / kGridNm) * kGridNm;
                    if ((cand[2] - cand[0]) * (cand[3] - cand[1]) < (double)m1_area) continue;
                    if (cand[0] > r[0] || cand[1] > r[1] || cand[2] < r[2] || cand[3] < r[3]) continue;
                    bool ok = true;
                    for (size_t j = 0; j < n && ok; ++j)
                        if (comp[j] != c && gap(cand, rs[j]) < (double)m1_space) ok = false;
                    if (!ok) continue;
                    rs[mem[0]] = cand;
                    for (size_t k = 1; k < mem.size(); ++k) rs[mem[k]] = cand;
                    break;
                }
            }

            // A via pad is taller than the wire that reaches it, so the pad's
            // overhang and the wire's edge leave a slit of the same net. Measured
            // on sg13g2_xnor2_2: a 160-tall run at y=1030 meets a 230-tall pad, and
            // the 35nm it leaves below the run reads as M1.b against a 180 rule.
            // The three shapes are one net and already touch, so closing the slit
            // cannot short anything; it is still checked against every other net.
            std::vector<Rect> fills;
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = 0; j < n; ++j) {
                    if (i == j || comp[i] != comp[j]) continue;
                    const Rect a = rs[i];
                    const Rect b = rs[j];
                    // overlapping on y, separated on x by less than the rule
                    const double yo0 = std::max(a[1], b[1]), yo1 = std::min(a[3], b[3]);
                    const double xgap = b[0] - a[2];
                    if (yo1 > yo0 && xgap > 0 && xgap < (double)m1_space) {
                        Rect fill{a[2], yo0, b[0], yo1};
                        bool ok = true;
                        for (size_t k = 0; k < n && ok; ++k)
                            if (comp[k] != comp[i] && gap(fill, rs[k]) < (double)m1_space) ok = false;
                        if (ok) fills.push_back(fill);
                        continue;
                    }
                    // overlapping on x, separated on y by less than the rule
                    const double xo0 = std::max(a[0], b[0]), xo1 = std::min(a[2], b[2]);
                    const double ygap = b[1] - a[3];
                    if (xo1 > xo0 && ygap > 0 && ygap < (double)m1_space) {
                        Rect fill{xo0, a[3], xo1, b[1]};
                        bool ok = true;
                        for (size_t k = 0; k < n && ok; ++k)
                            if (comp[k] != comp[i] && gap(fill, rs[k]) < (double)m1_space) ok = false;
                        if (ok) fills.push_back(fill);
                    }
                }
            }
            rs.insert(rs.end(), fills.begin(), fills.end());
        }
        }
    }

    if (bulk_planar(cfg)) apply_ihp_feedback(cfg, res, nets, no, rects, labels, m1_pin_rects);

    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) throw std::runtime_error("Cannot open GDS output: " + path);
    write_gds_header(f, cell_name, cfg);
    const double dbu_scale = 1.0 / cfg.gds_database_unit_nm;
    if (bulk_planar(cfg)) {
        write_merged_boundaries(f, rects, dbu_scale, gds_layer(cfg, "BOUNDARY"));
    } else {
        write_boundary_records(f, rects, dbu_scale, gds_layer(cfg, "BOUNDARY"));
    }
    write_extra_datatype_records(f, m1_pin_rects, gds_layer(cfg, "M1"), 2, dbu_scale);
    write_label_records(f, labels, dbu_scale);
    record(f, GDS_ENDSTR, {});
    record(f, GDS_ENDLIB, {});
    f.flush();
    if (!f) throw std::runtime_error("Cannot write GDS output: " + path);
    f.close();
    if (f.fail()) throw std::runtime_error("Cannot close GDS output: " + path);
}

}
