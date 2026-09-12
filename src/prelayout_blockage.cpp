// Builds pre-routing blockage rectangles (device geometry + power routing)
// and answers directional blocker queries near track points.
#include "prelayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <utility>

namespace aumedal {
namespace {
using Box = std::array<double, 4>;

std::vector<Box> merge_boxes(const std::vector<Box>& boxes) {
    const int n = (int)boxes.size();
    std::vector<int> parent(n);
    for (int i = 0; i < n; ++i) parent[i] = i;
    std::function<int(int)> find = [&](int a) { while (parent[a] != a) { parent[a] = parent[parent[a]]; a = parent[a]; } return a; };
    auto touch = [](const Box& a, const Box& b) {
        return a[0] <= b[2] && b[0] <= a[2] && a[1] <= b[3] && b[1] <= a[3];
    };
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            if (touch(boxes[i], boxes[j])) parent[find(i)] = find(j);
    std::map<int, Box> comp;
    for (int i = 0; i < n; ++i) {
        int r = find(i);
        if (!comp.count(r)) comp[r] = boxes[i];
        else {
            Box& c = comp[r];
            c[0] = std::min(c[0], boxes[i][0]); c[1] = std::min(c[1], boxes[i][1]);
            c[2] = std::max(c[2], boxes[i][2]); c[3] = std::max(c[3], boxes[i][3]);
        }
    }
    std::vector<Box> out;
    for (auto& kv : comp) out.push_back(kv.second);
    return out;
}

const Net* resolve_net(const std::vector<Net>& nets, const std::string& target) {
    for (const Net& n : nets) if (n.name == target) return &n;
    for (const Net& n : nets) if (n.name.find(target) != std::string::npos) return &n;
    return nullptr;
}

struct RowNets {
    const std::vector<long>& fin_top;
    const std::vector<long>& fin_bottom;
    const std::vector<std::string>& net_top;
    const std::vector<std::string>& net_bottom;
};

RowNets select_row_data(int row, const NetOrders& no) {
    bool flip = (row % 2);
    return flip
        ? RowNets{no.n_fins, no.p_fins, no.n_net, no.p_net}
        : RowNets{no.p_fins, no.n_fins, no.p_net, no.n_net};
}

using Seg = std::pair<std::array<long, 3>, std::array<long, 3>>;
using Seg2 = std::pair<std::array<long, 2>, std::array<long, 2>>;

int rdir_of(const Config& cfg, const std::string& layer) {
    int z = cfg.routing_layer_index(layer);
    return z >= 0 ? cfg.routing_directions[z] : BIDIRECTION;
}
bool in_power(const Config& cfg, const std::string& l) { return cfg.is_power_layer(l); }
std::array<double, 4> get_square(const Config& cfg, const Seg2& metal, const std::string& layer,
                                 const std::function<bool(double, double, double, double)>& active_overlap) {
    const long ch = cfg.cell_height;
    auto width = [&](const std::string& l) { return (double)cfg.width(l); };
    auto ext = [&](const std::string& l) { return cfg.rules.at("extension").at(l).get<double>(); };
    const long x0 = metal.first[0], y0 = metal.first[1], x1 = metal.second[0], y1 = metal.second[1];

    auto exts = [&](long y_value, double& x_ex, double& y_ex) {
        int rd = rdir_of(cfg, layer);
        if (y_value % ch == 0 && in_power(cfg, layer)) { x_ex = width(layer); y_ex = cfg.power_width(layer); }
        else {
            x_ex = (rd == HORIZONTAL) ? ext(layer) : width(layer);
            y_ex = (rd == VERTICAL) ? ext(layer) : width(layer);
        }
    };
    auto tr = [](double v) { return (double)(long)(v); };

    if (x0 == x1 && y0 == y1) {
        double x_ex, y_ex; exts(y0, x_ex, y_ex);
        return {(double)x0 - tr(x_ex / 2), (double)y0 - tr(y_ex / 2), (double)x0 + tr(x_ex / 2), (double)y0 + tr(y_ex / 2)};
    }
    if (x0 == x1) {
        double x_ex, y_ex; exts(y0, x_ex, y_ex);
        double ymin = std::min(y0, y1) - tr(y_ex / 2), ymax = std::max(y0, y1) + tr(y_ex / 2);
        return {(double)x0 - tr(x_ex / 2), ymin, (double)x0 + tr(x_ex / 2), ymax};
    }
    double x_ex, y_ex; exts(y0, x_ex, y_ex);
    double ymin = (double)y0 - tr(y_ex / 2), ymax = (double)y0 + tr(y_ex / 2);
    double xmin = (double)std::min(x0, x1) - tr(x_ex / 2), xmax = (double)std::max(x0, x1) + tr(x_ex / 2);
    if (layer == cfg.active_contact_layer) {
        double off = cfg.pitch.at("fin") - (width(layer) / 2 + width("M1") / 2);
        const std::string& pl = cfg.power_layer.back();
        long s2s = cfg.rules.at("spacing").at("S2S").at(pl).at(pl).get<long>();
        bool ov = active_overlap(xmin, ymin, xmax, ymax);
        if (y0 == cfg.power_width(pl) + s2s && ov) ymax += off;
        if (y0 == ch - cfg.power_width(pl) - s2s && ov) ymin -= off;
    }
    return {xmin, ymin, xmax, ymax};
}

std::vector<Seg2> continuous(const std::vector<Seg2>& metals, bool horizontal) {
    std::map<std::array<long, 2>, std::vector<std::array<long, 2>>> g;
    std::vector<std::array<long, 2>> nodes;
    for (const auto& m : metals) {
        bool ish = m.first[1] == m.second[1];
        bool isv = m.first[0] == m.second[0];
        if (horizontal ? !ish : !isv) continue;
        g[m.first].push_back(m.second);
        g[m.second].push_back(m.first);
    }
    std::vector<Seg2> out;
    std::set<std::array<long, 2>> visited;
    for (const auto& kv : g) {
        if (visited.count(kv.first)) continue;
        std::vector<std::array<long, 2>> path;
        std::vector<std::array<long, 2>> stack{kv.first};
        while (!stack.empty()) {
            auto cur = stack.back(); stack.pop_back();
            if (visited.count(cur)) continue;
            visited.insert(cur); path.push_back(cur);
            for (auto& nb : g[cur]) if (!visited.count(nb)) stack.push_back(nb);
        }
        std::sort(path.begin(), path.end());
        if (!path.empty()) out.push_back({path.front(), path.back()});
    }
    return out;
}

std::vector<int> walk_via_chain(const Config& cfg, const std::string& pl, int contact_z, int power_z) {
    std::vector<int> zs;
    const int step = (contact_z < power_z) ? 1 : -1;
    const auto& target_via = (step > 0) ? cfg.lower_via.at(pl) : cfg.upper_via.at(pl);
    std::set<std::optional<std::string>> seen_vias;
    int z = contact_z;
    while ((step > 0 && z < power_z) || (step < 0 && power_z < z)) {
        const auto& v = (step > 0) ? cfg.upper_via.at(cfg.routing_layers[z]) : cfg.lower_via.at(cfg.routing_layers[z]);
        if (seen_vias.count(v)) { z += step; continue; }
        seen_vias.insert(v); zs.push_back(z); z += step;
        if (v == target_via) { zs.push_back(power_z); break; }
    }
    return zs;
}

std::vector<Seg> collect_power_segments(const Config& cfg, const std::vector<Net>& nets) {
    std::vector<Seg> power_metals;
    const std::string& contact_layer = cfg.active_contact_layer;
    const int contact_z = cfg.routing_layer_index(contact_layer);

    for (const Net& net : nets) {
        if (!net.is_power || net.pins.size() < 2) continue;
        const Pin& contact = net.pins[0];
        const Pin& first_rail = net.pins[1];
        const long rail_y = first_rail.points[0].y;
        const Point* cp = nullptr;
        long best = -1;
        for (const Point& p : contact.points) {
            long d = std::labs(p.y - rail_y);
            if (cp == nullptr || d < best) { best = d; cp = &p; }
        }
        const std::array<long, 3> closest{cp->x, cp->y, cp->z};
        const std::array<long, 3> rail_point{cp->x, rail_y, cp->z};
        power_metals.push_back({closest, rail_point});

        for (const std::string& pl : cfg.power_layer) {
            if (pl == contact_layer) continue;
            const int power_z = cfg.routing_layer_index(pl);
            std::vector<int> zs = walk_via_chain(cfg, pl, contact_z, power_z);
            for (size_t i = 0; i + 1 < zs.size(); ++i)
                power_metals.push_back({{rail_point[0], rail_point[1], zs[i]}, {rail_point[0], rail_point[1], zs[i + 1]}});
        }
    }
    return power_metals;
}

struct ViaRec { Seg2 xy; bool add_via; std::string lower, upper; };

void group_metals_and_vias(const Config& cfg, const std::vector<Seg>& power_metals,
                           std::map<std::string, std::vector<Seg2>>& metals,
                           std::map<std::string, std::vector<ViaRec>>& vias) {
    const auto& layers = cfg.routing_layers;
    for (const Seg& m : power_metals) {
        Seg2 xy{{m.first[0], m.first[1]}, {m.second[0], m.second[1]}};
        for (int zn = 0; zn < (int)layers.size(); ++zn)
            if (m.first[2] == zn && m.second[2] == zn) metals[layers[zn]].push_back(xy);
        if (m.first[0] == m.second[0] && m.first[1] == m.second[1] && m.first[2] != m.second[2]) {
            const std::string& low = layers[m.first[2]];
            const std::string& up = layers[m.second[2]];
            auto via1 = cfg.upper_via.at(low);
            auto via2 = cfg.lower_via.at(up);
            if (via1.has_value() && !via1->empty()) {
                vias[*via1].push_back({xy, via1 == via2, low, up});
            }
        }
    }
}

void emit_power_rects(const Config& cfg, const std::map<std::string, std::vector<Seg2>>& metals,
                      const std::map<std::string, std::vector<ViaRec>>& vias,
                      std::map<int, std::vector<std::array<double, 4>>>& rects,
                      const std::function<int(const char*)>& lm,
                      const std::function<bool(double, double, double, double)>& active_overlap,
                      double cut_lo, double cut_hi) {
    auto add = [&](const std::string& layer, const std::array<double, 4>& sq) {
        double lx = sq[0], ly = std::max(sq[1], cut_lo), ux = sq[2], uy = std::min(sq[3], cut_hi);
        rects[lm(layer.c_str())].push_back({lx, ly, ux, uy});
    };
    for (const std::string& layer : cfg.routing_layers) {
        auto it = metals.find(layer);
        if (it == metals.end()) continue;
        auto xs = continuous(it->second, true), ys = continuous(it->second, false);
        for (auto& m : xs) add(layer, get_square(cfg, m, layer, active_overlap));
        for (auto& m : ys) add(layer, get_square(cfg, m, layer, active_overlap));
    }
    for (auto& kv : vias) {
        for (auto& vr : kv.second) {
            if (vr.add_via) add(kv.first, get_square(cfg, vr.xy, kv.first, active_overlap));
            add(vr.upper, get_square(cfg, vr.xy, vr.upper, active_overlap));
            add(vr.lower, get_square(cfg, vr.xy, vr.lower, active_overlap));
        }
    }
}

void power_routing(const Config& cfg, const std::vector<Net>& nets,
                   std::map<int, std::vector<std::array<double, 4>>>& rects,
                   const std::function<int(const char*)>& lm,
                   const std::function<bool(double, double, double, double)>& active_overlap,
                   double cut_lo, double cut_hi) {
    std::vector<Seg> power_metals = collect_power_segments(cfg, nets);
    std::map<std::string, std::vector<Seg2>> metals;
    std::map<std::string, std::vector<ViaRec>> vias;
    group_metals_and_vias(cfg, power_metals, metals, vias);
    emit_power_rects(cfg, metals, vias, rects, lm, active_overlap, cut_lo, cut_hi);
}
using AddFn = std::function<void(int layer, double lx, double ly, double ux, double uy, bool cut)>;
using LayerMapFn = std::function<int(const char*)>;
using OffsetFn = std::function<double(const char*, const char*)>;

void add_poly_blockage(const AddFn& add, const LayerMapFn& lm, long num_poly, double xoff, long pgate,
                       double gate_w, long ch, int row_count) {
    for (long x = 0; x < num_poly; ++x) {
        double cx = xoff + x * pgate;
        add(lm("Gate"), cx - gate_w / 2, 0, cx + gate_w / 2, (double)ch * row_count, false);
    }
}

void add_rail_blockage(const AddFn& add, const LayerMapFn& lm, const Config& cfg, int row_count,
                       long ch, long num_poly, long pgate) {
    for (int r = 0; r <= row_count; ++r) {
        double rail = (double)r * ch;
        for (const auto& pl : cfg.power_layer) {
            double pw = cfg.power_width(pl);
            add(lm(pl.c_str()), 0, rail - pw / 2, (double)num_poly * pgate, rail + pw / 2, false);
        }
    }
}

void add_active_blockage(const AddFn& add, const LayerMapFn& lm, const OffsetFn& off, const Config& cfg,
                         const NetOrders& no, int row_count, long ch, double xoff, double xunit,
                         double gate_w, long pfin, double y_off) {
    for (int row = 0; row < row_count; ++row) {
        RowNets rn = select_row_data(row, no);
        const auto& pf = rn.fin_top;
        const auto& nf = rn.fin_bottom;
        for (size_t x = 2; x < pf.size(); x += 2) {
            double cx = xoff + x * xunit;
            double minx = cx - gate_w / 2 - off("Gate", "Active"), maxx = cx + gate_w / 2 + off("Gate", "Active");
            double maxy = (double)(row + 1) * ch - y_off, miny = maxy - (double)pfin * pf[x];
            if (maxy - miny) add(lm("Active"), minx, miny, maxx, maxy, false);
            miny = (double)row * ch + y_off; maxy = miny + (double)pfin * nf[x];
            if (maxy - miny) add(lm("Active"), minx, miny, maxx, maxy, false);
        }
    }
}

void add_boundary_blockage(const AddFn& add, const LayerMapFn& lm, long num_poly, long pgate,
                           long ch, int row_count) {
    add(lm("BOUNDARY"), 0, 0, (double)num_poly * pgate, (double)ch * row_count, false);
}

void add_well_select_blockage(const AddFn& add, const LayerMapFn& lm, int row_count, long ch,
                              double npoff, long num_poly, long pgate) {
    for (int row = 0; row < row_count; ++row) {
        bool flip = (row % 2);
        double miny = (double)row * ch, maxy = (double)(row + 1) * ch;
        long p_min_y = (long)(miny + (maxy - miny) / 2 + (flip ? -npoff : npoff));
        add(lm("well"), 0, p_min_y, (double)num_poly * pgate, maxy, false);
        add(lm("Pselect"), 0, p_min_y, (double)num_poly * pgate, maxy, false);
        add(lm("Nselect"), 0, miny, (double)num_poly * pgate, p_min_y, false);
    }
}

void add_fin_blockage(const AddFn& add, const LayerMapFn& lm, const Config& cfg, int row_count,
                      long ch, long pfin, long num_poly, long pgate) {
    double maxx = (double)num_poly * pgate;
    long num_fin = (long)((double)ch * row_count / pfin);
    double yo = pfin / 2.0, fw = cfg.width("fin");
    for (long i = 0; i < num_fin; ++i) {
        double cy = yo + (double)i * pfin;
        add(lm("fin"), 0, cy - fw / 2, maxx, cy + fw / 2, false);
    }
}

void add_gcut_blockage(const AddFn& add, const LayerMapFn& lm, const OffsetFn& off, const Config& cfg,
                       const NetOrders& no, int row_count, long ch, double npoff, double xoff,
                       double xunit, double gate_w, long num_poly, long pgate) {
    double gw = cfg.width("GCut"), ogg = off("GCut", "Gate");
    for (int row = 0; row < row_count; ++row) {
        bool flip = (row % 2);
        double miny0 = (double)row * ch, maxy0 = (double)(row + 1) * ch;
        long cy = (long)(miny0 + (maxy0 - miny0) / 2 + (flip ? -npoff : npoff));
        RowNets rn = select_row_data(row, no);
        const auto& pno = rn.net_top;
        const auto& nno = rn.net_bottom;
        for (size_t x = 0; x < pno.size(); x += 2)
            if (pno[x] == DUMMY_NET && nno[x] == DUMMY_NET) {
                double cx = xoff + x * xunit;
                add(lm("GCut"), cx - gate_w / 2 - ogg, (double)cy - gw / 2, cx + gate_w / 2 + ogg, (double)cy + gw / 2, false);
            }
    }
    for (int row = 0; row <= row_count; ++row) {
        double rail = (double)row * ch;
        add(lm("GCut"), 0, rail - gw / 2, (double)num_poly * pgate, rail + gw / 2, false);
    }
}

void add_sdt_lisd_blockage(const AddFn& add, const LayerMapFn& lm, const Config& cfg, const NetOrders& no,
                           const std::vector<Net>& nets, int row_count, long ch, double xoff, double xunit,
                           long pfin, double y_off) {
    for (int pass = 0; pass < 2; ++pass) {
        const char* layer = pass ? "LISD" : "SDT";
        double w = cfg.width(layer);
        for (int row = 0; row < row_count; ++row) {
            RowNets rn = select_row_data(row, no);
            const auto& pf = rn.fin_top;
            const auto& nf = rn.fin_bottom;
            const auto& pno = rn.net_top;
            const auto& nno = rn.net_bottom;
            for (size_t x = 1; x < pf.size(); x += 2) {
                double cx = xoff + x * xunit, minx = cx - w / 2, maxx = cx + w / 2;
                const Net* pnet = resolve_net(nets, pno[x]);
                if (pnet && pnet->pins.size() > 1) {
                    double maxy = (double)(row + 1) * ch - y_off, miny = maxy - (double)pfin * pf[x];
                    if (maxy - miny) add(lm(layer), minx, miny, maxx, maxy, false);
                }
                const Net* nnet = resolve_net(nets, nno[x]);
                if (nnet && nnet->pins.size() > 1) {
                    double miny = (double)row * ch + y_off, maxy = miny + (double)pfin * nf[x];
                    if (maxy - miny) add(lm(layer), minx, miny, maxx, maxy, false);
                }
            }
        }
    }
}

}

void PreLayout::build_blockage(const Config& cfg, const NetOrders& no,
                               const std::vector<Net>& nets, long num_poly, int row_count) {
    rectangles.clear();
    LayerMapFn lm = [&](const char* n) { return cfg.specs.at("layer_map").at(n).get<int>(); };
    OffsetFn off = [&](const char* a, const char* b) { return cfg.rules.at("offset").at(a).at(b).get<double>(); };
    const long ch = cfg.cell_height;
    const double xoff = cfg.x_offset, xunit = cfg.x_unit, npoff = cfg.np_offset;
    const double gate_w = cfg.width("Gate");
    const long pgate = cfg.pitch.at("Gate"), pfin = cfg.pitch.at("fin");
    const double y_off = cfg.pitch.at("M1") - cfg.width("M1") / 2.0;
    const double cut_lo = -cfg.power_width(cfg.power_layer.back()) / 2.0;
    const double cut_hi = (double)row_count * ch + cfg.power_width(cfg.power_layer.back()) / 2.0;

    AddFn add = [&](int layer, double lx, double ly, double ux, double uy, bool cut) {
        if (cut) { ly = std::max(ly, cut_lo); uy = std::min(uy, cut_hi); }
        rectangles[layer].push_back({lx, ly, ux, uy});
    };

    add_poly_blockage(add, lm, num_poly, xoff, pgate, gate_w, ch, row_count);
    add_rail_blockage(add, lm, cfg, row_count, ch, num_poly, pgate);
    add_active_blockage(add, lm, off, cfg, no, row_count, ch, xoff, xunit, gate_w, pfin, y_off);
    add_boundary_blockage(add, lm, num_poly, pgate, ch, row_count);
    add_well_select_blockage(add, lm, row_count, ch, npoff, num_poly, pgate);
    add_fin_blockage(add, lm, cfg, row_count, ch, pfin, num_poly, pgate);
    add_gcut_blockage(add, lm, off, cfg, no, row_count, ch, npoff, xoff, xunit, gate_w, num_poly, pgate);
    add_sdt_lisd_blockage(add, lm, cfg, no, nets, row_count, ch, xoff, xunit, pfin, y_off);

    {
        const int an = lm("Active");
        auto active_overlap = [&](double lx, double ly, double ux, double uy) {
            for (const auto& r : rectangles[an])
                if (lx <= r[2] && r[0] <= ux && ly <= r[3] && r[1] <= uy) return true;
            return false;
        };
        LayerMapFn lmf = [&](const char* n) { return lm(n); };
        power_routing(cfg, nets, rectangles, lmf, active_overlap, cut_lo, cut_hi);
    }

    const int active_num = lm("Active");
    for (auto& kv : rectangles) {
        if (kv.first == active_num) continue;
        kv.second = merge_boxes(kv.second);
    }
}

namespace {
constexpr int LEFTd = 0, RIGHTd = 1, TOPd = 2, BOTTOMd = 3;
constexpr int TLd = 0, TRd = 1, BLd = 2, BRd = 3;
constexpr int SIDEm = 0, TIPm = 1, CORNERm = 2;
constexpr int H = 0, V = 1, BI = 2, PT = 3;

int metal_dir(double minx, double miny, double maxx, double maxy, double thr) {
    double w = maxx - minx, h = maxy - miny;
    if (w <= thr && h <= thr) return PT;
    if (w > thr && h <= thr) return H;
    if (h > thr && w <= thr) return V;
    return BI;
}
}

QueryResult PreLayout::query_polygon(const Config& cfg, const std::vector<long>& xs, const std::vector<long>& ys,
                                     int x_index, int y_index, double x_ex, double y_ex, const std::string& layer,
                                     int z, int mode, double dist) const {
    QueryResult r;
    const long x = xs[x_index], y = ys[y_index];
    const long right_x = x_index < (int)xs.size() - 1 ? xs[x_index + 1] : -1;
    const long left_x = x_index > 0 ? xs[x_index - 1] : -1;
    const long up_y = y_index < (int)ys.size() - 1 ? ys[y_index + 1] : -1;
    const long down_y = y_index > 0 ? ys[y_index - 1] : -1;
    const double hx = x_ex / 2, hy = y_ex / 2;
    const double in_lx = x - hx, in_ux = x + hx, in_ly = y - hy, in_uy = y + hy;

    constexpr long kDefaultMaxTipLenNm = 36;
    double max_tip_len;
    {
        const auto& mtl = cfg.rules.at("max_tip_len");
        if (mtl.contains(layer)) max_tip_len = mtl.at(layer).get<double>();
        else max_tip_len = (double)cfg.opt_long("default_max_tip_len", kDefaultMaxTipLenNm);
    }
    const int layer_num = cfg.specs.at("layer_map").at(layer).get<int>();
    auto it = rectangles.find(layer_num);
    if (it == rectangles.end() || it->second.empty()) return r;

    auto box_dist = [](double a_minx, double a_miny, double a_maxx, double a_maxy,
                       double b_minx, double b_miny, double b_maxx, double b_maxy) {
        double dx = std::max(0.0, std::max(b_minx - a_maxx, a_minx - b_maxx));
        double dy = std::max(0.0, std::max(b_miny - a_maxy, a_miny - b_maxy));
        return std::hypot(dx, dy);
    };

    for (const auto& poly : it->second) {
        const double pminx = poly[0], pminy = poly[1], pmaxx = poly[2], pmaxy = poly[3];
        const int pdir = metal_dir(pminx, pminy, pmaxx, pmaxy, max_tip_len);
        if (box_dist(in_lx, in_ly, in_ux, in_uy, pminx, pminy, pmaxx, pmaxy) >= dist) continue;
        const bool ox = (in_lx < pmaxx) && (in_ux > pminx);
        const bool oy = (in_ly < pmaxy) && (in_uy > pminy);

        if (mode == CORNERm) {
            if (!ox && !oy) {
                if (pminx > in_ux && pminy > in_uy) r.dir[TRd] = true;
                else if (pminx > in_ux && pmaxy < in_ly) r.dir[BRd] = true;
                else if (pmaxx < in_lx && pminy > in_uy) r.dir[TLd] = true;
                else if (pmaxx < in_lx && pmaxy < in_ly) r.dir[BLd] = true;
            }
            continue;
        }
        if (pminx > in_ux && oy) {
            if ((mode == SIDEm && (pdir == V || pdir == BI)) || (mode == TIPm && (pdir == H || pdir == PT))) r.dir[RIGHTd] = true;
            if (in_ly < pmaxy && pmaxy < in_uy) {
                for (int i = y_index - 1; i >= 0; --i) {
                    long py = ys[i]; double pym = py + hy;
                    if (pym < in_ly) break;
                    if (pym == pmaxy) r.not_edge = {{{x, py, z}, {right_x, py, z}}};
                }
            } else if (in_ly < pminy && pminy < in_uy) {
                for (int i = y_index + 1; i < (int)ys.size(); ++i) {
                    long ny = ys[i]; double nym = ny - hy;
                    if (in_uy < nym) break;
                    if (nym == pminy) r.not_edge = {{{x, ny, z}, {right_x, ny, z}}};
                }
            }
        } else if (pmaxx < in_lx && oy) {
            if ((mode == SIDEm && (pdir == V || pdir == BI)) || (mode == TIPm && (pdir == H || pdir == PT))) r.dir[LEFTd] = true;
            if (in_ly < pmaxy && pmaxy < in_uy) {
                for (int i = y_index - 1; i >= 0; --i) {
                    long py = ys[i]; double pym = py + hy;
                    if (pym < in_ly) break;
                    if (pym == pmaxy) r.not_edge = {{{left_x, py, z}, {x, py, z}}};
                }
            } else if (in_ly < pminy && pminy < in_uy) {
                for (int i = y_index + 1; i < (int)ys.size(); ++i) {
                    long ny = ys[i]; double nym = ny - hy;
                    if (in_uy < nym) break;
                    if (nym == pminy) r.not_edge = {{{left_x, ny, z}, {x, ny, z}}};
                }
            }
        } else if (pminy > in_uy && ox) {
            if ((mode == SIDEm && (pdir == H || pdir == BI)) || (mode == TIPm && (pdir == V || pdir == PT))) r.dir[TOPd] = true;
            if (in_lx < pmaxx && pmaxx < in_ux) {
                for (int i = x_index - 1; i >= 0; --i) {
                    long px = xs[i]; double pxm = px + hx;
                    if (pxm < in_lx) break;
                    if (pxm == pmaxx) r.not_edge = {{{px, y, z}, {px, up_y, z}}};
                }
            } else if (in_lx < pminx && pminx < in_ux) {
                for (int i = x_index + 1; i < (int)xs.size(); ++i) {
                    long nx = xs[i]; double nxm = nx - hx;
                    if (in_ux < nxm) break;
                    if (nxm == pminx) r.not_edge = {{{nx, y, z}, {nx, up_y, z}}};
                }
            }
        } else if (pmaxy < in_ly && ox) {
            if ((mode == SIDEm && (pdir == H || pdir == BI)) || (mode == TIPm && (pdir == V || pdir == PT))) r.dir[BOTTOMd] = true;
            if (in_lx < pmaxx && pmaxx < in_ux) {
                for (int i = x_index - 1; i >= 0; --i) {
                    long px = xs[i]; double pxm = px + hx;
                    if (pxm < in_lx) break;
                    if (pxm == pmaxx) r.not_edge = {{{px, down_y, z}, {px, y, z}}};
                }
            } else if (in_lx < pminx && pminx < in_ux) {
                for (int i = x_index + 1; i < (int)xs.size(); ++i) {
                    long nx = xs[i]; double nxm = nx - hx;
                    if (in_ux < nxm) break;
                    if (nxm == pminx) r.not_edge = {{{nx, down_y, z}, {nx, y, z}}};
                }
            }
        }
    }
    return r;
}

}
