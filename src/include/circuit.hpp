// Circuit: the parsed device lists and external pin set.
#pragma once
#include <set>
#include <string>
#include <vector>
#include "config.hpp"
#include "models.hpp"

namespace aumedal {

class Circuit {
public:
    std::string cell_name;
    std::vector<Mosfet> nfets;
    std::vector<Mosfet> pfets;
    std::set<std::string> ext_pins;

    void read_placement(const std::string& json_path);

    // Throws when a column's devices cannot fit the cell: taller than the
    // configured fin maximum, or tall enough that the two diffusions would
    // overlap. Both mean the placement needed folding it did not get.
    void validate_against(const Config& cfg) const;
};

}
