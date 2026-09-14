// Pre-placed Active geometry and the over-active overlap test.
#include <vector>
#include <utility>
#include <algorithm>
#include "prelayout.hpp"

namespace aumedal {

void PreLayout::add_active(const Config& cfg, const NetOrders& net_orders) {
    active.clear();
    const std::vector<long>& pnum = net_orders.p_fins;
    const std::vector<long>& nnum = net_orders.n_fins;

    const double gate_w = cfg.width("Gate");
    const double offset_ga = cfg.opt_bool("bulk_planar")
        ? cfg.opt_long("active_gate_extension")
        : cfg.rules.at("offset").at("Gate").at("Active").get<double>();
    const double fin_pitch = cfg.pitch.at("fin");
    const double y_off = cfg.opt_bool("bulk_planar")
        ? cfg.opt_long("active_y_offset")
        : cfg.pitch.at("M1") - cfg.width("M1") / 2.0;
    const double x_offset = cfg.x_offset;
    const double x_unit = cfg.x_unit;
    const long cell_height = cfg.cell_height;

    for (size_t x = 2; x < pnum.size(); x += 2) {
        double center_x = x_offset + x * x_unit;
        double min_x = center_x - gate_w / 2 - offset_ga;
        double max_x = center_x + gate_w / 2 + offset_ga;

        double pmax_y = cell_height - y_off;
        double pmin_y = pmax_y - fin_pitch * pnum[x];
        if (pmax_y - pmin_y) active.push_back({min_x, pmin_y, max_x, pmax_y});

        double nmin_y = y_off;
        double nmax_y = nmin_y + fin_pitch * nnum[x];
        if (nmax_y - nmin_y) active.push_back({min_x, nmin_y, max_x, nmax_y});
    }
}

bool PreLayout::is_over_active(const Config& cfg, long x, long y, long x_ex, long y_ex) const {
    return is_span_over_active(cfg, x, y, x, y, x_ex, y_ex);
}

bool PreLayout::is_box_inside_active(double x0, double y0, double x1, double y1) const {
    std::vector<double> edges{x0, x1};
    for (const auto& r : active) {
        if (r[0] > x0 && r[0] < x1) edges.push_back(r[0]);
        if (r[2] > x0 && r[2] < x1) edges.push_back(r[2]);
    }
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    for (size_t i = 0; i + 1 < edges.size(); ++i) {
        const double mid = (edges[i] + edges[i + 1]) / 2.0;
        std::vector<std::pair<double, double>> ys;
        for (const auto& r : active)
            if (r[0] <= mid && r[2] >= mid) ys.push_back({r[1], r[3]});
        std::sort(ys.begin(), ys.end());
        double reach = y0;
        for (const auto& s : ys) {
            if (s.first > reach) break;
            reach = std::max(reach, s.second);
            if (reach >= y1) break;
        }
        if (reach < y1) return false;
    }
    return true;
}

std::optional<std::pair<double, double>> PreLayout::active_band_at(double x, double y) const {
    std::vector<std::pair<double, double>> ys;
    for (const auto& r : active)
        if (r[0] <= x && x <= r[2]) ys.push_back({r[1], r[3]});
    std::sort(ys.begin(), ys.end());
    std::vector<std::pair<double, double>> merged;
    for (const auto& s : ys) {
        if (!merged.empty() && s.first <= merged.back().second)
            merged.back().second = std::max(merged.back().second, s.second);
        else
            merged.push_back(s);
    }
    for (const auto& s : merged)
        if (s.first <= y && y <= s.second) return s;
    return std::nullopt;
}

bool PreLayout::is_span_over_active(const Config&, long x0, long y0, long x1, long y1,
                                    long x_ex, long y_ex) const {
    double lx = std::min(x0, x1) - x_ex / 2.0, ux = std::max(x0, x1) + x_ex / 2.0;
    double ly = std::min(y0, y1) - y_ex / 2.0, uy = std::max(y0, y1) + y_ex / 2.0;
    for (const auto& r : active) {
        bool ox = (lx < r[2]) && (ux > r[0]);
        bool oy = (ly < r[3]) && (uy > r[1]);
        if (ox && oy) return true;
    }
    return false;
}

}
