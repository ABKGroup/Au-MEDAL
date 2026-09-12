// GDS writer interface.
#pragma once
#include <array>
#include <string>
#include <utility>
#include <vector>
#include "config.hpp"
#include "models.hpp"
#include "placement.hpp"
#include "prelayout.hpp"
#include "solve.hpp"

namespace aumedal {

// Geometry primitives also used by solve.cpp's post-solve DRC conflict
// checks, so those checks reconstruct exactly the shapes this file emits
// (via-enclosure squares, merged wire spans) instead of an independent,
// driftable re-derivation. Defined in gds.cpp.
using Rect = std::array<double, 4>;

Rect get_square(const Config& cfg, long x0, long y0, long x1, long y1,
                const std::string& layer, const PreLayout& pre);

std::vector<std::pair<std::array<long, 2>, std::array<long, 2>>>
merge_collinear(const std::vector<std::pair<std::array<long, 2>, std::array<long, 2>>>& edges);

void write_routing_gds(const std::string& path, const std::string& cell_name, const Config& cfg,
                       const RoutingResult& res, const PreLayout& pre, const NetOrders& no,
                       const std::vector<Net>& nets);

}
