// Core data types: Point, Pin, Net, Mosfet, and terminal constants.
#pragma once
#include <string>
#include <vector>

namespace aumedal {

constexpr const char* DUMMY_NET = "-";
constexpr const char* DUMMY_NAME = "DUMMY";

constexpr long TERM_DRAIN = 1, TERM_GATE = 2, TERM_SOURCE = 3, TERM_SD = (TERM_SOURCE | TERM_DRAIN);
constexpr long TERM_EXT_PIN = 4;
constexpr long TERM_ACCESS_POINT = 5;
constexpr const char* POWER_NET = "VDD";
constexpr const char* GND_NET = "VSS";

struct Mosfet {
    std::string name;
    std::string drain, gate, source;
    long nfin = 0;
};

struct Point {
    long x, y, z;
    bool operator<(const Point& o) const {
        return x != o.x ? x < o.x : (y != o.y ? y < o.y : z < o.z);
    }
    bool operator==(const Point& o) const { return x == o.x && y == o.y && z == o.z; }
};

struct Pin {
    long term = 0;
    std::vector<Point> points;
};

struct Net {
    std::string name;
    bool is_power = false;
    bool is_ext_pin = false;
    std::vector<Pin> pins;
};

}
