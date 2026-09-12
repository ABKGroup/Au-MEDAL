// Config: design rules, layer stack, and derived routing geometry.
#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "json.hpp"

namespace aumedal {

enum Direction { HORIZONTAL = 0, VERTICAL = 1, BIDIRECTION = 2 };

class Config {
public:
    nlohmann::json rules;
    nlohmann::json specs;
    nlohmann::json option;

    std::map<std::string, long> pitch;
    double np_offset = 0.0;
    double num_track = 0.0;
    double x_offset = 0.0, y_offset = 0.0;
    double x_unit = 0.0, y_unit = 0.0;

    std::vector<std::string> routing_layers;
    std::vector<int> routing_directions;
    std::vector<std::string> vias;
    std::map<std::string, std::optional<std::string>> lower_via, upper_via;

    long num_max_pmos_fins = 0, num_max_nmos_fins = 0;
    long cell_height = 0;

    std::string gate_contact_layer, active_contact_layer;
    std::vector<std::string> ext_pin_layer, power_layer;
    bool contact_over_active_gate = false;
    std::string power_net = "VDD", ground_net = "VSS";
    double gds_database_unit_nm = 0.25;
    std::vector<long> x_routing_resolution, y_routing_resolution;
    std::map<std::string, std::vector<std::string>> same_height_layers, upper_layers, lower_layers;

    bool opt_bool(const std::string& key, bool dflt = false) const { return option.value(key, dflt); }
    long opt_long(const std::string& key, long dflt = 0) const { return option.value(key, dflt); }
    std::string opt_str(const std::string& key, const std::string& dflt = "") const {
        if (!option.contains(key) || option.at(key).is_null()) return dflt;
        return option.at(key).get<std::string>();
    }

    int routing_layer_index(const std::string& name) const;

    void load(const std::string& json_path);

    long width(const std::string& layer) const { return rules.at("width").at(layer).get<long>(); }
    long power_width(const std::string& layer) const { return specs.at("power_width").at(layer).get<long>(); }
    bool is_power_layer(const std::string& layer) const {
        for (const auto& p : power_layer) if (p == layer) return true;
        return false;
    }
    std::optional<long> spacing_s2s(const std::string& a, const std::string& b) const;

    // Rows a gate contact may sit on. One in the middle when both devices in a
    // column share a gate net. When they do not, one per device row instead,
    // far enough apart that the two poly pads clear the GatPoly end to end
    // spacing, which is the closest they can legally be.
    std::vector<long> gate_contact_rows() const {
        const long mid = (long)(cell_height / 2.0 + np_offset);
        if (opt_bool("require_matching_gate_nets", true)) return {mid};
        const long sep = opt_long("gate_split_sep",
                         (long)(width("Cont") / 2 + opt_long("gate_contact_enclosure", 70)) +
                         (spacing_s2s("Gate", "Gate").value_or(180) / 2));
        return {mid - sep, mid, mid + sep};
    }
    bool is_gate_contact_row(long y) const {
        for (long r : gate_contact_rows()) if (r == y) return true;
        return false;
    }

private:
    void parse_scalar_specs();
    void parse_routing_layers();
    void parse_layer_adjacency();
    void parse_vias();
    void compute_pitch();
    void compute_derived_geometry();
};

}
