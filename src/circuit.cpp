// Parses the placement JSON into ordered NMOS/PMOS device lists.
#include "circuit.hpp"
#include <algorithm>
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

void Circuit::validate_against(const Config& cfg) const {
    const long nmax = cfg.num_max_nmos_fins, pmax = cfg.num_max_pmos_fins;
    const double y_off = (double)cfg.opt_long("active_y_offset");
    const double fin_pitch = cfg.pitch.count("fin") ? cfg.pitch.at("fin") : 1.0;
    std::vector<std::string> bad;
    const size_t ncol = std::min(nfets.size(), pfets.size());
    for (size_t i = 0; i < ncol; ++i) {
        const long nf = nfets[i].nfin, pf = pfets[i].nfin;
        std::string why;
        if (nmax > 0 && nf > nmax)
            why += " nfin=" + std::to_string(nf) + ">" + std::to_string(nmax);
        if (pmax > 0 && pf > pmax)
            why += " pfin=" + std::to_string(pf) + ">" + std::to_string(pmax);
        if (nf > 0 && pf > 0) {
            const double n_top = y_off + fin_pitch * (double)nf;
            const double p_bot = (double)cfg.cell_height - y_off - fin_pitch * (double)pf;
            if (n_top > p_bot)
                why += " diffusions overlap by " + std::to_string((long)(n_top - p_bot)) + "nm";
        }
        if (!why.empty())
            bad.push_back("column " + std::to_string(i) + " (" + nfets[i].name + "/" +
                          pfets[i].name + "):" + why);
    }
    if (bad.empty()) return;
    std::string msg = "placement does not fit the cell, these devices needed folding:";
    for (const auto& b : bad) msg += "\n  " + b;
    throw std::runtime_error(msg);
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
        // With no port list to go on, a net whose name is not a number is
        // taken for a port. That is a guess, and it puts internal nets in the
        // layout, so a caller that knows the ports overrides it below.
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
