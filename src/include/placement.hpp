// NetOrders and routing-track generation interfaces.
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "models.hpp"
#include "config.hpp"

namespace aumedal {

struct NetOrders {
    std::vector<std::string> n_net, p_net;
    std::vector<long> n_fins, p_fins;
};

NetOrders compute_net_orders(const std::vector<Mosfet>& nfets,
                             const std::vector<Mosfet>& pfets,
                             long diffusion_break);

std::vector<std::vector<long>> get_x_points(const Config& cfg, long width);

struct YPoints {
    std::vector<std::vector<long>> y_points;
    std::vector<std::vector<long>> ext_pin_y_tracks;
};
YPoints get_y_points(const Config& cfg, bool allow_below_min_track = false,
                     const NetOrders* no = nullptr);

// x shift the GDS writer applies to centre the gate span in a cell rounded up to whole sites.
inline double site_pad_shift(const Config& cfg, const NetOrders& no) {
    const long site = cfg.opt_long("site_width");
    if (site <= 0) return 0.0;
    const long np = ((long)no.p_net.size() + 1) / 2;
    const double gate_span = (double)np * cfg.pitch.at("Gate");
    return (std::ceil(gate_span / (double)site) * (double)site - gate_span) / 2.0;
}

// The P&R pin grid is on when either pitch is set, and the x pitch defaults to the site width.
inline bool pin_grid_active(const Config& cfg) {
    return cfg.opt_long("pin_grid_x", cfg.opt_long("site_width")) > 0 || cfg.opt_long("pin_grid_y", 0) > 0;
}

// The external pin row of row 0, the only row trusted to sit on the grid when no y pitch is configured.
inline long pin_grid_contact_row(const Config& cfg) {
    return (long)(cfg.cell_height / 2.0 + cfg.np_offset);
}

// True when the narrowest Metal1 drawn over router segment (x1,y1)-(x2,y2), or node when both ends agree, holds a pin grid point.
// (px, py) is that point clamped onto the segment, so a width-square pin centred there stays on the metal and still holds it.
inline bool pin_grid_hit(const Config& cfg, double shift, long x1, long y1, long x2, long y2, long& px, long& py) {
    const long gx = cfg.opt_long("pin_grid_x", cfg.opt_long("site_width"));
    const long gy = cfg.opt_long("pin_grid_y", 0);
    if (x1 != x2 && y1 != y2) return false;
    if (gy <= 0 && (y1 != y2 || y1 != pin_grid_contact_row(cfg))) return false;
    const long half = cfg.width(cfg.ext_pin_layer.empty() ? std::string("M1") : cfg.ext_pin_layer.back()) / 2;
    const long m = cfg.opt_long("pin_grid_margin", half - 5);
    auto pick = [m](long lo, long hi, long p, double off, long& out) {
        if (p <= 0) { out = lo; return true; }
        const double a = (double)lo + off - (double)m, b = (double)hi + off + (double)m;
        const double g = std::ceil(a / (double)p) * (double)p;
        if (g > b) return false;
        out = std::lround(std::min(std::max(g - off, (double)lo), (double)hi));
        return true;
    };
    return pick(std::min(x1, x2), std::max(x1, x2), gx, shift, px) &&
           pick(std::min(y1, y2), std::max(y1, y2), gy, 0.0, py);
}

}
