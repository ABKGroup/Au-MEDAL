// Enumerates the full SMT variable-name set for a routing graph.
#include "variables.hpp"

namespace aumedal {

std::set<std::string> all_variable_names(
    const std::set<std::array<long, 3>>& nodes,
    const std::set<std::pair<std::array<long, 3>, std::array<long, 3>>>& edges) {
    std::set<std::string> names;
    for (const auto& n : nodes)
        for (const auto& pre : metal_point_var_prefixes())
            names.insert(point_var_name(pre, n[0], n[1], n[2]));
    for (const auto& e : edges)
        names.insert(edge_var_name(e.first, e.second));
    return names;
}

}
