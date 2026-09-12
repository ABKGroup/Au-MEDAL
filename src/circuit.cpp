// Parses the placement JSON into ordered NMOS/PMOS device lists.
#include "circuit.hpp"
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include "json.hpp"

namespace aumedal {

using nlohmann::json;

namespace {

bool is_int(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (i == s.size()) return false;
    for (; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

struct ParsedDevice {
    std::string name;
    long fins;
    std::string drain, gate, source;
};

ParsedDevice parse_device(const json& obj, const std::string& which, const std::string& path) {
    if (!obj.is_object())
        throw std::runtime_error("Invalid '" + which + "' entry in " + path + ": expected object");
    std::string name = obj.value("name", std::string());
    if (name.empty()) throw std::runtime_error("Invalid '" + which + ".name' in " + path);
    long fins = obj.value("fin", 0L);
    if (fins < 0) throw std::runtime_error("Invalid '" + which + ".fin' in " + path);
    const auto& nets = obj.at("nets");
    if (!nets.is_array() || nets.size() != 3)
        throw std::runtime_error("Invalid '" + which + "' nets in " + path);
    return {name, fins, nets[0].get<std::string>(), nets[1].get<std::string>(), nets[2].get<std::string>()};
}

}

void Circuit::read_placement(const std::string& json_path) {
    std::ifstream f(json_path);
    if (!f) throw std::runtime_error("Cannot open placement: " + json_path);
    json data;
    f >> data;

    const auto& columns = data.at("columns");
    if (!columns.is_array() || columns.empty())
        throw std::runtime_error("Invalid placement JSON (missing/non-list 'columns'): " + json_path);

    std::vector<ParsedDevice> n_parsed, p_parsed;
    for (const auto& col : columns) {
        if (!col.is_object())
            throw std::runtime_error("Invalid column entry in " + json_path);
        ParsedDevice n = parse_device(col.at("nmos"), "nmos", json_path);
        ParsedDevice p = parse_device(col.at("pmos"), "pmos", json_path);
        for (const std::string* net : {&n.drain, &n.gate, &n.source, &p.drain, &p.gate, &p.source})
            if (!is_int(*net)) ext_pins.insert(*net);
        n_parsed.push_back(std::move(n));
        p_parsed.push_back(std::move(p));
    }

    std::unordered_set<std::string> used_names;
    auto build = [&](const std::vector<ParsedDevice>& parsed, std::vector<Mosfet>& out) {
        for (const auto& d : parsed) {
            if (d.name == "dummy") {
                out.push_back(Mosfet{DUMMY_NAME, DUMMY_NET, DUMMY_NET, DUMMY_NET, 0});
                continue;
            }
            long idx = 0;
            while (used_names.count(d.name + "_" + std::to_string(idx))) ++idx;
            std::string fet_name = d.name + "_" + std::to_string(idx);
            used_names.insert(fet_name);
            out.push_back(Mosfet{fet_name, d.drain, d.gate, d.source, d.fins});
        }
    };
    build(n_parsed, nfets);
    build(p_parsed, pfets);
}

}
