// RoutingGraph: grid nodes and edges.
#pragma once
#include <array>
#include <set>
#include <utility>
#include <vector>
#include "config.hpp"
#include "prelayout.hpp"

namespace aumedal {

struct RoutingGraph {
    std::set<std::array<long, 3>> nodes;
    std::set<std::pair<std::array<long, 3>, std::array<long, 3>>> edges;
};

inline bool edge_in_graph(const RoutingGraph& g, const std::array<long, 3>& a,
                          const std::array<long, 3>& b) {
    auto e = (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
    return g.edges.count(e) > 0;
}
inline std::set<std::array<long, 3>> points_with_edges(const RoutingGraph& g) {
    std::set<std::array<long, 3>> s;
    for (const auto& e : g.edges) { s.insert(e.first); s.insert(e.second); }
    return s;
}

RoutingGraph build_routing_graph(const Config& cfg,
                                 const std::vector<std::vector<long>>& x_tracks,
                                 const std::vector<std::vector<long>>& y_tracks,
                                 const std::vector<std::vector<long>>& ext_pin_y_tracks,
                                 const PreLayout& pre);

}
