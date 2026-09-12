// RoutingResult and the solver entry points.
#pragma once
#include <array>
#include <utility>
#include <vector>
#include "circuit.hpp"
#include "config.hpp"
#include "graph.hpp"
#include "models.hpp"
#include "placement.hpp"
#include "prelayout.hpp"

namespace aumedal {

struct ViaEnc { long x, y, z; int dir; int level; };
struct ExtPin { std::string net; long x, y, z; };

struct RoutingResult {
    bool sat = false;
    std::string top_layer;
    long tolerance = -1;
    std::vector<std::pair<std::array<long, 3>, std::array<long, 3>>> metals;
    std::vector<ViaEnc> via_enc;
    std::vector<ExtPin> ext_pins;
    std::vector<long> objective_values;
    long via_count = 0;
    long total_length = 0;
    long num_constraints = 0;
};

RoutingResult solve_cell(const Config& cfg,
                         const std::vector<std::vector<long>>& x_points,
                         const std::vector<std::vector<long>>& y_points,
                         const std::vector<std::vector<long>>& ext_pin_y_tracks,
                         const RoutingGraph& g,
                         std::vector<Net>& cnets,
                         PreLayout& pre,
                         const NetOrders& no,
                         const std::string& debug_dump_default = "");

RoutingResult solve_router(const Config& cfg, const Circuit& circ, const NetOrders& no,
                           const std::string& debug_dump_default = "");

}
