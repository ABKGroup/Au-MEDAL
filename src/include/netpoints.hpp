// Net/pin point construction interface.
#pragma once
#include <vector>
#include "config.hpp"
#include "models.hpp"
#include "placement.hpp"
#include "circuit.hpp"

namespace aumedal {

std::vector<Net> compute_net_points(const Config& cfg, const Circuit& circuit,
                                    const NetOrders& net_orders,
                                    const std::vector<std::vector<long>>& y_points);

}
