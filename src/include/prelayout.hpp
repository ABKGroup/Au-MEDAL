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
    // Same test over the whole segment between two track positions, so a
    // wire that only passes over Active is caught as well as one that
    // ends on it.
    bool is_span_over_active(const Config& cfg, long x0, long y0, long x1, long y1,
                             long x_ex, long y_ex) const;
    // Is the whole box inside the union of the active rectangles. Overlap is
    // not enough for a shape that has to be enclosed.
    bool is_box_inside_active(double x0, double y0, double x1, double y1) const;
    // The merged device y interval covering (x, y), used to find how far a
    // contact pad pokes out of the diffusion it sits on.
    std::optional<std::pair<double, double>> active_band_at(double x, double y) const;

    void build_blockage(const Config& cfg, const NetOrders& net_orders,
                        const std::vector<Net>& nets, long num_poly, int row_count);

    QueryResult query_polygon(const Config& cfg, const std::vector<long>& xs, const std::vector<long>& ys,
                              int x_index, int y_index, double x_ex, double y_ex, const std::string& layer,
                              int z, int mode, double dist) const;
};

}
