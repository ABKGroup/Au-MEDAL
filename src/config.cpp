// Loads the config JSON and derives the design rules the router depends on.
#include "config.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

namespace aumedal {

using nlohmann::json;

int Config::routing_layer_index(const std::string& name) const {
    for (size_t i = 0; i < routing_layers.size(); ++i)
        if (routing_layers[i] == name) return static_cast<int>(i);
    return -1;
}

std::optional<long> Config::spacing_s2s(const std::string& a, const std::string& b) const {
    const auto& s2s = rules.at("spacing").at("S2S");
    auto ia = s2s.find(a);
    if (ia == s2s.end()) return std::nullopt;
    auto ib = ia->find(b);
    if (ib == ia->end()) return std::nullopt;
    return ib->get<long>();
}

static std::optional<std::string> opt_string(const json& v) {
    if (v.is_null()) return std::nullopt;
    return v.get<std::string>();
}

static std::vector<std::string> get_list(const json& o, const char* k) {
    return o.contains(k) && !o.at(k).is_null() ? o.at(k).get<std::vector<std::string>>()
                                               : std::vector<std::string>{};
}

void Config::parse_scalar_specs() {
    cell_height = specs.at("cell_height").get<long>();
    gate_contact_layer = specs.at("gate_contact_layer").get<std::string>();
    active_contact_layer = specs.at("active_contact_layer").get<std::string>();
    ext_pin_layer = specs.at("ext_pin_layer").get<std::vector<std::string>>();
    power_layer = specs.at("power_layer").get<std::vector<std::string>>();
    contact_over_active_gate = specs.value("contact_over_active_gate",
                                           rules.value("contact_over_active_gate", false));

    power_net = opt_str("power_net_name", "VDD");
    ground_net = opt_str("ground_net_name", "VSS");
    gds_database_unit_nm = specs.value("gds_database_unit_nm",
                                       option.value("gds_database_unit_nm", 0.25));

    const auto& mf = specs.at("max_fins");
    num_max_pmos_fins = mf.at("pmos").get<long>();
    num_max_nmos_fins = mf.at("nmos").get<long>();
}

void Config::parse_routing_layers() {
    static const std::map<std::string, int> dir_map = {{"B", BIDIRECTION}, {"H", HORIZONTAL}, {"V", VERTICAL}};
    for (const auto& lo : specs.at("routing_layers")) {
        const std::string name = lo.at("layer_name").get<std::string>();
        routing_layers.push_back(name);
        if (lo.contains("direction")) {
            auto it = dir_map.find(lo.at("direction").get<std::string>());
            routing_directions.push_back(it == dir_map.end() ? -1 : it->second);
        }
        if (lo.contains("lower_via")) lower_via[name] = opt_string(lo.at("lower_via"));
        if (lo.contains("upper_via")) upper_via[name] = opt_string(lo.at("upper_via"));
        x_routing_resolution.push_back(lo.value("x_routing_resolution", 1L));
        y_routing_resolution.push_back(lo.value("y_routing_resolution", 1L));
    }
}

void Config::parse_layer_adjacency() {
    for (const auto& lo : specs.at("routing_layers")) {
        const std::string name = lo.at("layer_name").get<std::string>();
        if (lo.contains("lower_layers")) lower_layers[name] = get_list(lo, "lower_layers");
        if (lo.contains("upper_layers")) upper_layers[name] = get_list(lo, "upper_layers");
        if (lo.contains("same_height_layers")) same_height_layers[name] = get_list(lo, "same_height_layers");
    }
}

void Config::parse_vias() {
    for (const auto& vo : specs.at("vias")) {
        const std::string name = vo.at("via_name").get<std::string>();
        vias.push_back(name);
        if (vo.contains("lower_layers")) lower_layers[name] = get_list(vo, "lower_layers");
        if (vo.contains("upper_layers")) upper_layers[name] = get_list(vo, "upper_layers");
    }
    for (const auto& l : routing_layers) same_height_layers[l].push_back(l);
    for (const auto& v : vias) same_height_layers[v].push_back(v);
}

void Config::compute_pitch() {
    std::set<std::string> width_keys, s2s_keys;
    for (auto it = rules.at("width").begin(); it != rules.at("width").end(); ++it) width_keys.insert(it.key());
    for (auto it = rules.at("spacing").at("S2S").begin(); it != rules.at("spacing").at("S2S").end(); ++it) s2s_keys.insert(it.key());
    for (const auto& layer : width_keys) {
        if (!s2s_keys.count(layer)) continue;
        auto self_s2s = spacing_s2s(layer, layer);
        if (!self_s2s) {
            std::cerr << "Warning: Ignore " << layer << " pitch calculation: no self S2S spacing.\n";
            continue;
        }
        pitch[layer] = width(layer) + *self_s2s;
    }
}

void Config::compute_derived_geometry() {
    np_offset = (num_max_nmos_fins - num_max_pmos_fins) / 2.0 * pitch.at("fin");
    num_track = static_cast<double>(cell_height) / pitch.at("M1");
    x_offset = pitch.at("Gate") / 2.0;
    y_offset = (power_width("M1") - width("M1")) / 2.0;
    x_unit = pitch.at("Gate") / 2.0;
    y_unit = static_cast<double>(pitch.at("M1"));
}

static json read_json_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot find JSON file: " + path);
    json d;
    f >> d;
    return d;
}

void Config::load(const std::string& json_path) {
    json data = read_json_file(json_path);

    rules = data.value("design_rules", json::object());
    specs = data.value("design_specs", json::object());

    option = json::object();
    auto merge_into_option = [this](const json& src) {
        if (src.is_object())
            for (auto it = src.begin(); it != src.end(); ++it) option[it.key()] = it.value();
    };
    merge_into_option(data.value("design_options", json::object()));
    merge_into_option(specs.value("design_options", json::object()));
    merge_into_option(specs.value("design_option", json::object()));

    parse_scalar_specs();
    parse_routing_layers();
    parse_layer_adjacency();
    parse_vias();
    compute_pitch();
    compute_derived_geometry();
}

}
