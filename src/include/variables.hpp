// Deterministic SMT variable naming for points, edges, and commodities.
#pragma once
#include <array>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace aumedal {

inline const std::vector<std::string>& metal_point_var_prefixes() {
    static const std::vector<std::string> p = {
        "G",
        "SIDE_R", "SIDE_L", "SIDE_T", "SIDE_B",
        "TIP_R", "TIP_L", "TIP_T", "TIP_B",
        "CORNER_TL", "CORNER_TR", "CORNER_BL", "CORNER_BR",
        "VIA_ENC_HOR_L", "VIA_ENC_HOR_U", "VIA_ENC_VER_L", "VIA_ENC_VER_U",
        "METAL_HOR", "METAL_VER",
    };
    return p;
}

inline std::string coord3(const std::array<long, 3>& p) {
    return "x" + std::to_string(p[0]) + "y" + std::to_string(p[1]) + "z" + std::to_string(p[2]);
}

inline std::string point_var_name(const std::string& prefix, long x, long y, long z) {
    const std::string coord = coord3({x, y, z});
    if (prefix == "G") return "G_" + coord;
    return prefix + "_G_" + coord;
}

inline std::string net_point_var_name(const std::string& net, long x, long y, long z) {
    return "G_" + net + "_" + coord3({x, y, z});
}
inline std::string net_edge_var_name(const std::string& net, const std::array<long, 3>& a,
                                     const std::array<long, 3>& b) {
    const std::array<long, 3>& u = (a < b) ? a : b;
    const std::array<long, 3>& v = (a < b) ? b : a;
    return "E_" + net + "_" + coord3(u) + "_" + coord3(v);
}
inline std::string comm_edge_var_name(const std::string& net, int c, const std::array<long, 3>& a,
                                      const std::array<long, 3>& b) {
    const std::array<long, 3>& u = (a < b) ? a : b;
    const std::array<long, 3>& v = (a < b) ? b : a;
    return "E_" + net + "_C" + std::to_string(c) + "_" + coord3(u) + "_" + coord3(v);
}
inline std::string ext_pin_chain_var_name(const std::string& net, long x, long curr_y, long z) {
    return "EXT_PIN_" + net + "_" + std::to_string(x) + "_" + std::to_string(curr_y) + "_" + std::to_string(z);
}

inline std::string edge_var_name(const std::array<long, 3>& a, const std::array<long, 3>& b) {
    const std::array<long, 3>& u = (a < b) ? a : b;
    const std::array<long, 3>& v = (a < b) ? b : a;
    return "E_" + coord3(u) + "_" + coord3(v);
}

std::set<std::string> all_variable_names(
    const std::set<std::array<long, 3>>& nodes,
    const std::set<std::pair<std::array<long, 3>, std::array<long, 3>>>& edges);

}
