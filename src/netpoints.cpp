// Builds each net's pin point sets from the placement and track grid.
#include "netpoints.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <stdexcept>

namespace aumedal {

namespace {

struct Builder {
    const Config& cfg;
    const std::set<std::string>& ext_pins;
    const std::vector<std::vector<long>>& y_points;
    std::map<std::string, Net> nets;
    std::vector<std::string> order;

    static std::string to_upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }

    Net& net_ref(const std::string& name, bool is_ext) {
        auto it = nets.find(name);
        if (it == nets.end()) {
            Net n;
            n.name = name;
            const std::string up = to_upper(name);
            const std::string pu = to_upper(cfg.power_net), gu = to_upper(cfg.ground_net);
            n.is_power = up.find(pu) != std::string::npos || up.find(gu) != std::string::npos;
            n.is_ext_pin = is_ext;
            order.push_back(name);
            it = nets.emplace(name, std::move(n)).first;
        }
        return it->second;
    }

    std::string create_power_name(const std::string& base) {
        long idx = 0;
        while (nets.count(base + "_" + std::to_string(idx))) ++idx;
        return base + "_" + std::to_string(idx);
    }

    void add_power_pin(Net& net, long term, long x, long rail_y) {
        for (const auto& pl : cfg.power_layer) {
            int layer_idx = cfg.routing_layer_index(pl);
            Pin pin; pin.term = term;
            pin.points.push_back({x, rail_y, layer_idx});
            net.pins.push_back(std::move(pin));
        }
    }

    void add_pin_to_net(std::string net_name, long term, long x, const std::vector<long>& ys,
                        int layer, bool pn_flip, long gnd_rail, long power_rail) {
        bool is_power = net_name == cfg.power_net;
        bool is_gnd = net_name == cfg.ground_net;
        if (is_power || is_gnd) net_name = create_power_name(net_name);

        Net& net = net_ref(net_name, ext_pins.count(net_name) > 0);
        Pin pin; pin.term = term;
        std::set<Point> uniq;
        for (long y : ys) uniq.insert({x, y, layer});
        pin.points.assign(uniq.begin(), uniq.end());
        net.pins.push_back(std::move(pin));

        if ((is_power && !pn_flip) || (is_gnd && pn_flip))
            add_power_pin(net, term, x, power_rail);
        else if ((is_gnd && !pn_flip) || (is_power && pn_flip))
            add_power_pin(net, term, x, gnd_rail);
    }

    void add_gate_points(const std::string& nn_n, const std::string& nn_p,
                         long x, long min_y, long max_y, int layer, bool pn_flip) {
        const auto& ys = y_points[layer];
        long width_m1 = cfg.width("M1");
        long spacing = *cfg.spacing_s2s("M1", "M1");
        double y_offset = cfg.y_offset;
        long max_routing_y = (long)(max_y - width_m1 - spacing - y_offset);
        long min_routing_y = (long)(min_y + width_m1 + spacing + y_offset);
        std::vector<long> gate_ys;
        for (long y : ys) if (min_routing_y <= y && y <= max_routing_y) gate_ys.push_back(y);

        long gnd_rail = min_y, power_rail = max_y;
        bool same_net = nn_n == nn_p;
        bool has_dummy = nn_n == DUMMY_NET || nn_p == DUMMY_NET;
        if (same_net || has_dummy) {
            std::string name = (nn_n != DUMMY_NET) ? nn_n : nn_p;
            add_pin_to_net(name, TERM_GATE, x, gate_ys, layer, pn_flip, gnd_rail, power_rail);
            return;
        }
        if (!cfg.contact_over_active_gate)
            throw std::runtime_error("gate cannot be placed over active region (no gate matching)");
        throw std::runtime_error("contact_over_active_gate N/P split path not yet ported");
    }

    void add_source_drain_points(const std::string& nn_n, const std::string& nn_p, long nfin_n,
                                 long nfin_p, long x, long min_y, long max_y, int layer, bool pn_flip) {
        const auto& ys = y_points[layer];
        long gnd_rail = min_y, power_rail = max_y;
        double ac_w = cfg.width(cfg.active_contact_layer);
        double y_offset = cfg.pitch.at("M1") - cfg.width("M1") / 2.0;

        auto collect_active_overlap_ys = [&](double active_min, double active_max) {
            std::vector<long> out;
            for (long y : ys) {
                double metal_max = y + ac_w / 2, metal_min = y - ac_w / 2;
                if (!(metal_max < active_min || metal_min > active_max)) out.push_back(y);
            }
            return out;
        };
        if (nn_n != DUMMY_NET) {
            double len = cfg.pitch.at("fin") * (double)nfin_n;
            double active_min = y_offset, active_max = active_min + len;
            add_pin_to_net(nn_n, TERM_SD, x, collect_active_overlap_ys(active_min, active_max),
                           layer, pn_flip, gnd_rail, power_rail);
        }
        if (nn_p != DUMMY_NET) {
            double len = cfg.pitch.at("fin") * (double)nfin_p;
            double active_max = cfg.cell_height - y_offset, active_min = active_max - len;
            add_pin_to_net(nn_p, TERM_SD, x, collect_active_overlap_ys(active_min, active_max),
                           layer, pn_flip, gnd_rail, power_rail);
        }
    }
};

}

std::vector<Net> compute_net_points(const Config& cfg, const Circuit& circuit,
                                    const NetOrders& net_orders,
                                    const std::vector<std::vector<long>>& y_points) {
    std::set<std::string> ext_pins(circuit.ext_pins.begin(), circuit.ext_pins.end());
    Builder b{cfg, ext_pins, y_points, {}, {}};

    const long x_unit = (long)cfg.x_unit;
    const long min_y = 0, max_y = cfg.cell_height;
    const bool pn_flip = false;
    int gate_layer = cfg.routing_layer_index(cfg.gate_contact_layer);
    int ac_layer = cfg.routing_layer_index(cfg.active_contact_layer);

    if (net_orders.n_net.size() != net_orders.p_net.size())
        throw std::runtime_error("net_orders length mismatch");

    for (size_t xi = 0; xi < net_orders.n_net.size(); ++xi) {
        const std::string& nn_n = net_orders.n_net[xi];
        const std::string& nn_p = net_orders.p_net[xi];
        long nfin_n = net_orders.n_fins[xi], nfin_p = net_orders.p_fins[xi];
        int layer = (xi % 2) ? ac_layer : gate_layer;
        long x_coord = (long)(cfg.x_offset + (long)xi * x_unit);
        if (xi % 2 == 0)
            b.add_gate_points(nn_n, nn_p, x_coord, min_y, max_y, layer, pn_flip);
        else
            b.add_source_drain_points(nn_n, nn_p, nfin_n, nfin_p, x_coord, min_y, max_y, layer, pn_flip);
    }

    std::vector<Net> out;
    out.reserve(b.order.size());
    for (const auto& name : b.order) out.push_back(b.nets.at(name));
    return out;
}

}
