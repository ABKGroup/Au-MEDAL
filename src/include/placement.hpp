// NetOrders and routing-track generation interfaces.
#pragma once
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
YPoints get_y_points(const Config& cfg, bool allow_below_min_track = false);

}
