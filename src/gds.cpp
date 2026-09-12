// Emits the routed and device geometry as rectangles and writes the GDSII.
#include "gds.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace aumedal {
namespace {

using Rect = std::array<double, 4>;
struct GdsLabel { std::string text; double x, y; int layer; };

long ext_of(const Config& cfg, const std::string& layer) {
    return cfg.rules.at("extension").at(layer).get<long>();
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

bool over_active(const PreLayout& pre, const Rect& box) {
    for (const auto& a : pre.active)
        if (box[0] <= a[2] && box[2] >= a[0] && box[1] <= a[3] && box[3] >= a[1]) return true;
    return false;
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
    if (x0 == x1 && y0 == y1) {
        exts(y0, x_ex, y_ex);
        return {(double)(x0 - x_ex / 2), (double)(y0 - y_ex / 2), (double)(x0 + x_ex / 2), (double)(y0 + y_ex / 2)};
    }
    if (x0 == x1) {
        exts(y0, x_ex, y_ex);
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

using MetalEdge = std::pair<std::array<long, 3>, std::array<long, 3>>;

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

using LisdExtents = std::map<double, std::vector<std::pair<double, double>>>;

LisdExtents compute_lisd_fixed_extents(const Config& cfg, const NetOrders& no, const std::vector<Net>& nets) {
    LisdExtents out;
    const long ch = cfg.cell_height;
    const double x_off = cfg.x_offset;
    const double fin_pitch = cfg.pitch.at("fin");
    const double y_off = cfg.pitch.at("M1") - cfg.width("M1") / 2.0;
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
            if (via1.has_value() && !via1->empty() && via1 == via2)
                add(gds_layer(cfg, *via1), get_square(cfg, x, y, x, y, *via1, pre));
            add(gds_layer(cfg, layers[uz]), get_square(cfg, x, y, x, y, layers[uz], pre));
            add(gds_layer(cfg, layers[lz]), get_square(cfg, x, y, x, y, layers[lz], pre));
        }
    }
    for (auto& kv : by_layer) {
        const std::string& layer = layers[kv.first];
        auto spans = merge_collinear(kv.second);
        if (lisd_fixed != nullptr && layer == cfg.active_contact_layer && !spans.empty()) {
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

void emit_gates(const AddLayerFn& add, const Config& cfg, long num_poly, double x_off,
                double gate_pitch, long ch) {
    for (long x = 0; x < num_poly; ++x) {
        const double cx = x_off + x * gate_pitch;
        add("Gate", {cx - cfg.width("Gate") / 2.0, 0, cx + cfg.width("Gate") / 2.0, (double)ch}, false);
    }
}

void emit_fins(const AddLayerFn& add, const Config& cfg, long ch, double fin_pitch, double max_x) {
    const long num_fin = (long)(ch / fin_pitch);
    for (long i = 0; i < num_fin; ++i) {
        const double cy = fin_pitch / 2.0 + i * fin_pitch;
        add("fin", {0, cy - cfg.width("fin") / 2.0, max_x, cy + cfg.width("fin") / 2.0}, false);
    }
}

void emit_power_rails(const AddLayerFn& add, const Config& cfg, std::vector<GdsLabel>& labels,
                      long ch, double max_x) {
    for (long r = 0; r <= 1; ++r) {
        const double rail = r * (double)ch;
        for (const std::string& pl : cfg.power_layer) {
            const double pw = cfg.power_width(pl);
            add(pl, {0, rail - pw / 2.0, max_x, rail + pw / 2.0}, false);
            if (has_layer(cfg, cfg.power_layer.back())) {
                const std::string power = (r % 2 == 1) ? cfg.power_net : cfg.ground_net;
                labels.push_back({power, max_x / 2.0, rail, gds_layer(cfg, cfg.power_layer.back())});
            }
        }
    }
}

void emit_well_select(const AddLayerFn& add, const Config& cfg, std::vector<GdsLabel>& labels,
                      long ch, double max_x) {
    const double p_min_y = (long)(ch / 2.0 + cfg.np_offset);
    add("well", {0, p_min_y, max_x, (double)ch}, false);
    add("Pselect", {0, p_min_y, max_x, (double)ch}, false);
    add("Nselect", {0, 0, max_x, p_min_y}, false);
    if (has_layer(cfg, "well")) labels.push_back({cfg.power_net, max_x / 2.0, p_min_y + (ch - p_min_y) / 2.0, gds_layer(cfg, "well")});
    if (has_layer(cfg, "P_SUB")) labels.push_back({cfg.ground_net, max_x + cfg.pitch.at("M1"), p_min_y + (ch - p_min_y) / 2.0, gds_layer(cfg, "P_SUB")});
}

void emit_gate_cuts(const AddLayerFn& add, const Config& cfg, const NetOrders& no,
                    long ch, double x_off, double max_x) {
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
    for (long x = 2; x < ncol; x += 2) {
        const double cx = x_off + x * cfg.x_unit;
        const double mnx = cx - cfg.width("Gate") / 2.0 - off(cfg, "Gate", "Active");
        const double mxx = cx + cfg.width("Gate") / 2.0 + off(cfg, "Gate", "Active");
        auto [pmin, pmax] = p_fin_extent(ch, y_off, fin_pitch, no.p_fins[x]);
        if (pmax != pmin) add("Active", {mnx, pmin, mxx, pmax}, false);
        auto [nmin, nmax] = n_fin_extent(y_off, fin_pitch, no.n_fins[x]);
        if (nmax != nmin) add("Active", {mnx, nmin, mxx, nmax}, false);
    }
}

void emit_sdt_lisd_tracks(const AddLayerFn& add, const Config& cfg, const NetOrders& no,
                          const std::vector<Net>& nets, long ch, double x_off,
                          double fin_pitch, double y_off) {
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
    const double max_x = num_poly * gate_pitch;
    const double y_off = cfg.pitch.at("M1") - cfg.width("M1") / 2.0;
    AddLayerFn add = [&](const std::string& lname, Rect r, bool cut) {
        if (!has_layer(cfg, lname)) return;
        if (cut) clamp_rect(r, clamp_lo, clamp_hi);
        rects[gds_layer(cfg, lname)].push_back(r);
    };

    emit_gates(add, cfg, num_poly, x_off, gate_pitch, ch);
    emit_fins(add, cfg, ch, fin_pitch, max_x);
    emit_power_rails(add, cfg, labels, ch, max_x);
    add("BOUNDARY", {0, 0, max_x, (double)ch}, false);
    emit_well_select(add, cfg, labels, ch, max_x);
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
                            double dbu_scale) {
    for (const auto& kv : rects)
        for (const auto& r : kv.second) {
            record(f, GDS_BOUNDARY, {});
            { std::vector<uint8_t> d; put16(d, (uint16_t)kv.first); record(f, GDS_LAYER, d); }
            { std::vector<uint8_t> d; put16(d, 0); record(f, GDS_DATATYPE, d); }
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
                         int texttype, double dbu_scale) {
    for (const auto& lb : labels) {
        record(f, GDS_TEXT, {});
        { std::vector<uint8_t> d; put16(d, (uint16_t)lb.layer); record(f, GDS_LAYER, d); }
        { std::vector<uint8_t> d; put16(d, (uint16_t)texttype); record(f, GDS_TEXTTYPE, d); }
        { std::vector<uint8_t> d; put32(d, (int32_t)std::lround(lb.x * dbu_scale)); put32(d, (int32_t)std::lround(lb.y * dbu_scale)); record(f, GDS_XY, d); }
        record(f, GDS_STRING, str_data(lb.text));
        record(f, GDS_ENDEL, {});
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
        const long enc = cfg.rules.at("enclosure").at(*via).at(layer).get<long>();
        long xe, ye, xmw, ymw;
        if (v.dir == HORIZONTAL) { xe = enc; ye = 0; xmw = metal_ex; ymw = metal_width; }
        else { xe = 0; ye = enc; xmw = metal_width; ymw = metal_ex; }
        const double lx = (xe != 0) ? v.x - (via_width / 2.0 + xe) : v.x - xmw / 2.0;
        const double ux = (xe != 0) ? v.x + (via_width / 2.0 + xe) : v.x + xmw / 2.0;
        const double ly = (ye != 0) ? v.y - (via_width / 2.0 + ye) : v.y - ymw / 2.0;
        const double uy = (ye != 0) ? v.y + (via_width / 2.0 + ye) : v.y + ymw / 2.0;
        add_cut(gds_layer(cfg, layer), {lx, ly, ux, uy});
    }
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
    std::map<std::string, ExtPin> ext_by_net;
    for (const auto& p : res.ext_pins) ext_by_net[p.net] = p;
    for (const auto& kv : ext_by_net) {
        const auto& p = kv.second;
        labels.push_back({p.net, (double)p.x, (double)p.y, gds_layer(cfg, layers[p.z])});
    }

    build_device_geometry(cfg, no, nets, pre, rects, labels, clamp_lo, clamp_hi, lisd_fixed);

    for (auto& kv : rects) kv.second = union_rects(kv.second);

    std::ofstream f(path, std::ios::binary);
    write_gds_header(f, cell_name, cfg);
    const double dbu_scale = 1.0 / cfg.gds_database_unit_nm;
    write_boundary_records(f, rects, dbu_scale);
    const int texttype = cfg.option.value("pin_label_texttype", 251);
    write_label_records(f, labels, texttype, dbu_scale);
    record(f, GDS_ENDSTR, {});
    record(f, GDS_ENDLIB, {});
}

}
