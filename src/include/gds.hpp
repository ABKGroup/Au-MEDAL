// GDS writer interface.
#pragma once
#include <string>
#include <vector>
#include "config.hpp"
#include "models.hpp"
#include "placement.hpp"
#include "prelayout.hpp"
#include "solve.hpp"

namespace aumedal {

void write_routing_gds(const std::string& path, const std::string& cell_name, const Config& cfg,
                       const RoutingResult& res, const PreLayout& pre, const NetOrders& no,
                       const std::vector<Net>& nets);

}
