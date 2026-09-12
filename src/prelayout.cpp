// Pre-placed Active geometry and the over-active overlap test.
#include "prelayout.hpp"

namespace aumedal {

void PreLayout::add_active(const Config& cfg, const NetOrders& net_orders) {
    active.clear();
    const std::vector<long>& pnum = net_orders.p_fins;
    const std::vector<long>& nnum = net_orders.n_fins;

    const double gate_w = cfg.width("Gate");
    const double offset_ga = cfg.rules.at("offset").at("Gate").at("Active").get<double>();
    const double fin_pitch = cfg.pitch.at("fin");
    const double y_off = cfg.pitch.at("M1") - cfg.width("M1") / 2.0;
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

bool PreLayout::is_over_active(const Config&, long x, long y, long x_ex, long y_ex) const {
    double lx = x - x_ex / 2.0, ux = x + x_ex / 2.0;
    double ly = y - y_ex / 2.0, uy = y + y_ex / 2.0;
    for (const auto& r : active) {
        bool ox = (lx < r[2]) && (ux > r[0]);
        bool oy = (ly < r[3]) && (uy > r[1]);
        if (ox && oy) return true;
    }
    return false;
}

}
