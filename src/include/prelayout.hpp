// PreLayout: pre-placed geometry the routing graph and blockages query.
#pragma once
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "config.hpp"
#include "models.hpp"
#include "placement.hpp"

namespace aumedal {

struct QueryResult {
    std::array<bool, 4> dir{{false, false, false, false}};
    std::optional<std::array<std::array<long, 3>, 2>> not_edge;
};

class PreLayout {
public:
    std::vector<std::array<double, 4>> active;

    std::map<int, std::vector<std::array<double, 4>>> rectangles;

    void add_active(const Config& cfg, const NetOrders& net_orders);
    bool is_over_active(const Config& cfg, long x, long y, long x_ex, long y_ex) const;

    void build_blockage(const Config& cfg, const NetOrders& net_orders,
                        const std::vector<Net>& nets, long num_poly, int row_count);

    QueryResult query_polygon(const Config& cfg, const std::vector<long>& xs, const std::vector<long>& ys,
                              int x_index, int y_index, double x_ex, double y_ex, const std::string& layer,
                              int z, int mode, double dist) const;
};

}
