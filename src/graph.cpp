// Builds the routing grid graph (in-layer and via edges) over the tracks.
#include "graph.hpp"
#include <algorithm>

namespace aumedal {

namespace {
std::pair<std::array<long,3>, std::array<long,3>> mk_edge(const std::array<long,3>& a,
                                                          const std::array<long,3>& b) {
    return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
}

}

RoutingGraph build_routing_graph(const Config& cfg,
                                 const std::vector<std::vector<long>>& x_tracks,
                                 const std::vector<std::vector<long>>& y_tracks,
                                 const std::vector<std::vector<long>>& ext_pin_y_tracks,
                                 const PreLayout& pre) {
    RoutingGraph g;
    const auto& layers = cfg.routing_layers;
    const auto& dirs = cfg.routing_directions;
    const bool allow_contact_over_active_gate = cfg.contact_over_active_gate;
    const bool ext_h_edges = cfg.opt_bool("ext_pin_horizontal_edges");
    const bool ext_via_edges = cfg.opt_bool("ext_pin_via_edges");

    auto add_edge = [&](const std::array<long, 3>& a, const std::array<long, 3>& b) {
        g.nodes.insert(a); g.nodes.insert(b); g.edges.insert(mk_edge(a, b));
    };
    auto ext_pin_has = [&](int layer, long y) {
        const auto& v = ext_pin_y_tracks[layer];
        return std::find(v.begin(), v.end(), y) != v.end();
    };

    for (int li = 0; li < (int)layers.size(); ++li) {
        const std::string& layer = layers[li];
        int dir = dirs[li];
        long ext = cfg.rules.at("extension").at(layer).get<long>();
        long w = cfg.width(layer);

        for (size_t xi = 0; xi < x_tracks[li].size(); ++xi) {
            long x = x_tracks[li][xi];
            for (size_t yi = 0; yi < y_tracks[li].size(); ++yi) {
                long y = y_tracks[li][yi];
                bool is_ext_pin = ext_pin_has(li, y);

                long x_full_extent, y_full_extent;
                if (dir == HORIZONTAL) { x_full_extent = ext; y_full_extent = w; }
                else if (dir == VERTICAL) { x_full_extent = w; y_full_extent = ext; }
                else { x_full_extent = w; y_full_extent = w; }

                bool is_over_active = pre.is_over_active(cfg, x, y, x_full_extent, y_full_extent);
                bool is_power_rail = cfg.is_power_layer(layer) && (y % cfg.cell_height == 0);

                if (layer == cfg.gate_contact_layer && !allow_contact_over_active_gate && is_over_active)
                    continue;

                if (xi > 0 && (dir == HORIZONTAL || dir == BIDIRECTION || is_power_rail)) {
                    long prev_x = x_tracks[li][xi - 1];
                    bool prev_over = pre.is_over_active(cfg, prev_x, y, x_full_extent, y_full_extent);
                    bool add_x = true;
                    if (layer == cfg.gate_contact_layer && !allow_contact_over_active_gate && prev_over) add_x = false;
                    if (layer == cfg.active_contact_layer && is_over_active && prev_over) add_x = false;
                    if (!ext_h_edges && is_ext_pin) add_x = false;
                    if (add_x) add_edge({prev_x, y, li}, {x, y, li});
                }
                if (yi > 0 && (dir == VERTICAL || dir == BIDIRECTION)) {
                    long prev_y = y_tracks[li][yi - 1];
                    bool prev_over = pre.is_over_active(cfg, x, prev_y, x_full_extent, y_full_extent);
                    bool add_y = true;
                    if (layer == cfg.gate_contact_layer && !allow_contact_over_active_gate && prev_over) add_y = false;
                    if (add_y) add_edge({x, prev_y, li}, {x, y, li});
                }
                if (li > 0) {
                    auto it = cfg.lower_layers.find(layer);
                    if (it == cfg.lower_layers.end()) continue;
                    for (const auto& lower : it->second) {
                        int lo = cfg.routing_layer_index(lower);
                        if (lo < 0) continue;
                        bool add_via = true;
                        const auto& lx = x_tracks[lo];
                        const auto& ly = y_tracks[lo];
                        if (std::find(lx.begin(), lx.end(), x) == lx.end() ||
                            std::find(ly.begin(), ly.end(), y) == ly.end())
                            add_via = false;
                        if (lower == cfg.gate_contact_layer && !allow_contact_over_active_gate && is_over_active) add_via = false;
                        if (!ext_via_edges && is_ext_pin) add_via = false;
                        if (add_via) add_edge({x, y, lo}, {x, y, li});
                    }
                }
            }
        }
    }
    return g;
}

}
