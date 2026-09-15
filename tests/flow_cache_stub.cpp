// Exercise the production CLI/cache with deterministic solve/write boundaries.
#include "../src/flow.cpp"
#include <cstdlib>

namespace aumedal {
NetOrders compute_net_orders(const std::vector<Mosfet>&, const std::vector<Mosfet>&, long) {
    return {};
}
YPoints get_y_points(const Config&, bool, const NetOrders*) { return {}; }
std::vector<Net> compute_net_points(const Config&, const Circuit&, const NetOrders&,
                                  const std::vector<std::vector<long>>&) { return {}; }
void PreLayout::add_active(const Config&, const NetOrders&) {}

RoutingResult solve_router(const Config&, const Circuit&, const NetOrders&, const std::string&) {
    std::cout << "[TEST] solve_router\n";
    RoutingResult r;
    r.sat = true;
    if (const char* status = std::getenv("AUMEDAL_TEST_STATUS")) {
        r.sat = false;
        r.undecided = std::string(status) != "unsat";
        r.geometry_unresolved = std::string(status) == "geometry";
    }
    r.top_layer = "M1";
    r.tolerance = 0;
    r.metals.push_back({{0, 0, 0}, {100, 0, 0}});
    return r;
}

nlohmann::json routing_result_to_json(const RoutingResult& r) {
    return {{"sat", r.sat}, {"top_layer", r.top_layer}, {"tolerance", r.tolerance},
            {"metals", r.metals}};
}
RoutingResult routing_result_from_json(const nlohmann::json& j) {
    RoutingResult r;
    r.sat = j.value("sat", false);
    r.top_layer = j.value("top_layer", std::string());
    r.tolerance = j.value("tolerance", -1L);
    if (j.contains("metals")) r.metals = j.at("metals").get<decltype(r.metals)>();
    return r;
}

void write_routing_gds(const std::string& path, const std::string& cell_name, const Config& cfg,
                       const RoutingResult& r, const PreLayout&, const NetOrders&,
                       const std::vector<Net>&) {
    std::cout << "[TEST] write_routing_gds\n";
    if (const char* failure = std::getenv("AUMEDAL_TEST_FAIL_WRITER"))
        if (std::string(failure) == "before_open")
            throw std::runtime_error("injected GDS writer failure before open");
    std::ofstream out;
    out.exceptions(std::ios::failbit | std::ios::badbit);
    out.open(path, std::ios::binary);
    out << "test emitter " << cell_name << '\n';
    if (std::getenv("AUMEDAL_TEST_FAIL_WRITER")) {
        out.close();
        throw std::runtime_error("injected GDS writer failure");
    }
    out << cfg.rules.dump() << '\n' << routing_result_to_json(r).dump() << '\n';
    out.close();
}
}
