// SmtModel: z3 variable interning and all constraint-generation entry points.
#pragma once
#include <array>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "z3++.h"
#include "config.hpp"
#include "graph.hpp"
#include "models.hpp"
#include "prelayout.hpp"

namespace aumedal {

class SmtModel {
public:
    z3::context ctx;
    std::unordered_map<std::string, z3::expr> vars;
    long tolerance = 0;

    z3::expr bool_var(const std::string& name) {
        auto it = vars.find(name);
        if (it != vars.end()) return it->second;
        z3::expr e = ctx.bool_const(name.c_str());
        vars.emplace(name, e);
        return e;
    }

    std::vector<z3::expr> tip_side_exclusivity(const std::set<std::array<long, 3>>& nodes);

    std::vector<z3::expr> metal_direction(const std::vector<std::vector<long>>& x_points,
                                             const std::vector<std::vector<long>>& y_points,
                                             const RoutingGraph& g);

    std::vector<z3::expr> tip_helper(const Config& cfg,
                                        const std::vector<std::vector<long>>& x_points,
                                        const std::vector<std::vector<long>>& y_points,
                                        const RoutingGraph& g);

    std::vector<z3::expr> side_helper(const Config& cfg,
                                         const std::vector<std::vector<long>>& x_points,
                                         const std::vector<std::vector<long>>& y_points,
                                         const RoutingGraph& g);

    std::vector<z3::expr> corner_helper(const Config& cfg,
                                           const std::vector<std::vector<long>>& x_points,
                                           const std::vector<std::vector<long>>& y_points,
                                           const RoutingGraph& g);

    std::vector<z3::expr> via_enclosure(const Config& cfg,
                                           const std::vector<std::vector<long>>& x_points,
                                           const std::vector<std::vector<long>>& y_points,
                                           const RoutingGraph& g);

    std::vector<z3::expr> cross_layer_via_block(const Config& cfg,
                                                   const std::vector<std::vector<long>>& x_points,
                                                   const std::vector<std::vector<long>>& y_points,
                                                   const RoutingGraph& g);

    std::vector<z3::expr> minimum_area(const Config& cfg,
                                          const std::vector<std::vector<long>>& x_points,
                                          const std::vector<std::vector<long>>& y_points,
                                          const RoutingGraph& g);

    std::vector<z3::expr> add_side_to_side_spacing(const Config& cfg,
                                                      const std::vector<std::vector<long>>& x_points,
                                                      const std::vector<std::vector<long>>& y_points,
                                                      const RoutingGraph& g);
    std::vector<z3::expr> add_tip_to_tip_spacing(const Config& cfg,
                                                    const std::vector<std::vector<long>>& x_points,
                                                    const std::vector<std::vector<long>>& y_points,
                                                    const RoutingGraph& g);
    std::vector<z3::expr> add_side_to_tip_spacing(const Config& cfg,
                                                     const std::vector<std::vector<long>>& x_points,
                                                     const std::vector<std::vector<long>>& y_points,
                                                     const RoutingGraph& g);
    std::vector<z3::expr> add_corner_spacing(const Config& cfg,
                                                const std::vector<std::vector<long>>& x_points,
                                                const std::vector<std::vector<long>>& y_points,
                                                const RoutingGraph& g);
    std::vector<z3::expr> add_via_spacing(const Config& cfg,
                                             const std::vector<std::vector<long>>& x_points,
                                             const std::vector<std::vector<long>>& y_points,
                                             const RoutingGraph& g);

    std::vector<z3::expr> add_layer_exclusivity(const Config& cfg, const std::vector<std::vector<long>>& x_points,
                                                   const std::vector<std::vector<long>>& y_points,
                                                   const RoutingGraph& g, const std::vector<Net>& nets);
    std::vector<z3::expr> add_edge_assignment(const Config& cfg, const RoutingGraph& g, const std::vector<Net>& nets);
    std::vector<z3::expr> add_vertex_exclusivity(const Config& cfg, const RoutingGraph& g, const std::vector<Net>& nets);
    std::vector<z3::expr> add_commodity_flow(const Config& cfg, const RoutingGraph& g, const std::vector<Net>& nets);
    std::vector<z3::expr> add_cuts(const Config& cfg, const RoutingGraph& g, const std::vector<Net>& nets);
    std::vector<z3::expr> add_metal_segment(const Config& cfg, const RoutingGraph& g, const std::vector<Net>& nets);
    std::vector<z3::expr> add_pin_grid_lock(const Config& cfg, const RoutingGraph& g, const std::vector<Net>& nets);
    std::vector<z3::expr> add_pre_layout_blockage(const Config& cfg, const std::vector<std::vector<long>>& x_points,
                                                     const std::vector<std::vector<long>>& y_points,
                                                     const RoutingGraph& g, const PreLayout& pre);
    std::vector<z3::expr> add_minimum_pin_length(const Config& cfg, const std::vector<std::vector<long>>& x_points,
                                                    const std::vector<std::vector<long>>& y_points,
                                                    const RoutingGraph& g, const std::vector<Net>& nets);

    std::vector<z3::expr> via_helper(const Config& cfg,
                                        const std::vector<std::vector<long>>& x_points,
                                        const std::vector<std::vector<long>>& y_points,
                                        const RoutingGraph& g);
};

void add_ext_pin_points(std::vector<Net>& nets, const Config& cfg,
                        const std::vector<std::vector<long>>& x_points,
                        const std::vector<std::vector<long>>& ext_pin_y_tracks,
                        long tolerance = 0);

void add_access_points(std::vector<Net>& nets, const Config& cfg,
                       const std::vector<std::vector<long>>& x_points,
                       const std::vector<std::vector<long>>& y_points,
                       long tolerance = 0);

}
