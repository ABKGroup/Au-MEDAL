// Circuit: the parsed device lists and external pin set.
#pragma once
#include <set>
#include <string>
#include <vector>
#include "models.hpp"

namespace aumedal {

class Circuit {
public:
    std::string cell_name;
    std::vector<Mosfet> nfets;
    std::vector<Mosfet> pfets;
    std::set<std::string> ext_pins;

    void read_placement(const std::string& json_path);
};

}
