// Builds the routing grid graph (in-layer and via edges) over the tracks.
#include "graph.hpp"
#include <algorithm>
#include <vector>
#include <utility>

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
                    // A Gate-layer edge that runs across Active draws poly
                    // over the diffusion, forming an unintended parasitic
                    // transistor. Found via sg13g2_nor3_4/nand3_2, and again
                    // on sg13g2_a21oi_4 once the gate layer stopped carrying
                    // a track at every source/drain column. Ask about the
                    // whole segment: an edge can clear Active at both of its
                    // endpoints and still pass straight over a power tie in
                    // between. The real gate columns are drawn separately by
                    // emit_gates(), not through this graph edge.
                    // Inflate by the GatPoly-to-Activ minimum (Gat.d,
                    // 70nm per side) so a rail that merely comes too close
                    // to a diffusion is blocked along with one crossing it.
                    if (layer == "Gate" &&
                        pre.is_span_over_active(cfg, prev_x, y, x, y,
                                                x_full_extent + 140, y_full_extent + 140))
                        add_x = false;
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
                        // A drawn gate contact needs its Cont 140 away from
                        // every diffusion (Cnt.e) and its poly pad clear of
                        // them. Only a gate-contact row may carry one, and a
                        // split gate column has one per device row besides
                        // the middle one. Those outer rows clear a diffusion
                        // only where the devices near them are narrow, so
                        // test the Cont grown by Cnt.e against every
                        // diffusion, not the bare gate square.
                        if (lower == cfg.gate_contact_layer && cfg.opt_bool("bulk_planar") &&
                            (!cfg.is_gate_contact_row(y) ||
                             pre.is_over_active(cfg, x, y,
                                                cfg.width("Cont") + 2 * cfg.opt_long("gate_contact_active_space", 140),
                                                cfg.width("Cont") + 2 * cfg.opt_long("gate_contact_active_space", 140))))
                            add_via = false;
                        // An active contact draws a 300 island of Activ around
                        // its Cont (Cnt.c). Where that island rises out of the
                        // diffusion it leaves a free wall, and on a contacted
                        // gate pitch that wall lands 45 from the next gate
                        // column, against a Gat.d minimum of 70. Allow the
                        // island only where every wall it exposes faces no gate.
                        if (lower == cfg.active_contact_layer && cfg.opt_bool("bulk_planar") &&
                            (y % (long)cfg.cell_height) != 0) {
                            const double h = cfg.width("Cont") / 2.0 +
                                             (double)cfg.opt_long("active_contact_enclosure");
                            const double lo = (double)y - h, hi = (double)y + h;
                            // The diffusion runs that cover the island's whole
                            // height. A run is what the island may sit in; its
                            // two ends are where a wall would be exposed.
                            std::vector<std::pair<double, double>> runs;
                            for (const auto& r : pre.active)
                                if (r[1] <= lo && r[3] >= hi) runs.push_back({r[0], r[2]});
                            std::sort(runs.begin(), runs.end());
                            std::vector<std::pair<double, double>> merged;
                            for (const auto& s : runs) {
                                if (!merged.empty() && s.first <= merged.back().second)
                                    merged.back().second = std::max(merged.back().second, s.second);
                                else merged.push_back(s);
                            }
                            // emit_gates drops the poly on a column with no
                            // device on either row, and the cell has none past
                            // its outermost column, so an overhang there faces
                            // nothing. Anywhere a gate is drawn the island has
                            // to stay inside the diffusion.
                            auto gate_drawn = [&](double gx) {
                                for (const auto& r : pre.active)
                                    if (r[0] <= gx && gx <= r[2]) return true;
                                return false;
                            };
                            bool ok = false;
                            for (const auto& m : merged) {
                                if ((double)x < m.first || (double)x > m.second) continue;
                                ok = ((double)x - h >= m.first ||
                                      !gate_drawn((double)x - cfg.x_unit)) &&
                                     ((double)x + h <= m.second ||
                                      !gate_drawn((double)x + cfg.x_unit));
                                break;
                            }
                            if (!ok) add_via = false;
                        }
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
