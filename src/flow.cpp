// flow: the routing-only cell-generation CLI.
#include <cctype>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.hpp"
#include "circuit.hpp"
#include "placement.hpp"
#include "netpoints.hpp"
#include "prelayout.hpp"
#include "solve.hpp"
#include "gds.hpp"

namespace {

bool path_exists(const std::string& p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0;
}

void make_dirs(const std::string& path) {
    std::string acc;
    auto ensure = [&](const std::string& dir) {
        if (dir.empty() || dir == "/" || path_exists(dir)) return;
        if (::mkdir(dir.c_str(), 0755) != 0 && !path_exists(dir))
            throw std::runtime_error("failed to create directory: " + dir);
    };
    for (char c : path) {
        if (c == '/') ensure(acc);
        acc += c;
    }
    ensure(acc);
}

struct Logger {
    std::ofstream file;
    bool file_logging_enabled = true;

    static int level_rank(const std::string& name) {
        if (name == "DEBUG") return 10;
        if (name == "INFO") return 20;
        if (name == "WARNING" || name == "WARN") return 30;
        if (name == "ERROR") return 40;
        if (name == "CRITICAL") return 50;
        return 20;
    }

    void set_level(const std::string& name) {
        std::string up = name;
        for (char& c : up) c = (char)std::toupper((unsigned char)c);
        file_logging_enabled = (level_rank(up) <= 20);
    }

    void open(const std::string& p) { file.open(p, std::ios::app); }

    void step(const std::string& msg) {
        std::cout << "[STEP] " << msg << "\n";
        if (!file.is_open() || !file_logging_enabled) return;
        char ts[32];
        std::time_t t = std::time(nullptr);
        std::strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        file << ts << " - INFO - " << msg << "\n";
        file.flush();
    }
};

aumedal::RoutingResult route_cell(const aumedal::Config& cfg, const aumedal::Circuit& circ,
                                  const std::string& cell_name, const std::string& gds_out,
                                  const std::string& routing_cache_path, bool regen_gds_only) {
    const long diff_break = cfg.specs.at("diffusion_break").get<long>();
    aumedal::NetOrders no = aumedal::compute_net_orders(circ.nfets, circ.pfets, diff_break);
    auto yp = aumedal::get_y_points(cfg, cfg.opt_bool("allow_below_min_track"), &no);
    auto cnets = aumedal::compute_net_points(cfg, circ, no, yp.y_points);
    aumedal::PreLayout pre;
    pre.add_active(cfg, no);
    const std::string kGdsSuffix = ".gds";
    std::string smt2_default = gds_out;
    if (smt2_default.size() > kGdsSuffix.size() &&
        smt2_default.compare(smt2_default.size() - kGdsSuffix.size(), kGdsSuffix.size(), kGdsSuffix) == 0)
        smt2_default = smt2_default.substr(0, smt2_default.size() - kGdsSuffix.size()) + ".smt2";

    aumedal::RoutingResult r;
    if (regen_gds_only) {
        std::ifstream cache_in(routing_cache_path);
        if (!cache_in)
            throw std::runtime_error("--regen_gds_only: no cached routing result at " + routing_cache_path +
                                     " (run once without --regen_gds_only first)");
        nlohmann::json cache_json;
        cache_in >> cache_json;
        r = aumedal::routing_result_from_json(cache_json);
    } else {
        r = aumedal::solve_router(cfg, circ, no, smt2_default);
        if (r.sat && !routing_cache_path.empty()) {
            std::ofstream cache_out(routing_cache_path);
            cache_out << aumedal::routing_result_to_json(r).dump(2);
        }
    }

    if (r.sat && !gds_out.empty())
        aumedal::write_routing_gds(gds_out, cell_name, cfg, r, pre, no, cnets);
    return r;
}

void print_usage(const char* prog) {
    std::cout
        << "Usage: " << prog
        << " --save_dir DIR --cell_name NAME --config CFG.json --placement_file PL.json\n"
        << "                  [--no_cache] [--regen_gds_only] [--ports A,B,Y,VDD,VSS] [--log_file PATH]\n\n"
        << "Routing-only standard-cell generator (pure C++). Writes\n"
        << "<save_dir>/<cell_name>/<cell_name>.gds on SAT, ROUTING_UNSAT.txt on UNSAT.\n";
}

}

int main(int argc, char** argv) try {
    std::string save_dir, cell_name, config_path, placement_file, schematic, log_file, ports_csv;
    bool no_cache = false;
    bool regen_gds_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto val = [&](const char* name) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string("missing value after ") + name);
            return argv[++i];
        };
        if (a == "--save_dir") save_dir = val("--save_dir");
        else if (a == "--cell_name") cell_name = val("--cell_name");
        else if (a == "--config") config_path = val("--config");
        else if (a == "--placement_file") placement_file = val("--placement_file");
        else if (a == "--schematic") schematic = val("--schematic");
        else if (a == "--no_cache") no_cache = true;
        else if (a == "--regen_gds_only") regen_gds_only = true;
        else if (a == "--ports") ports_csv = val("--ports");
        else if (a == "--log_file") log_file = val("--log_file");
        else if (a == "-h" || a == "--help") { print_usage(argv[0]); return 0; }
        else throw std::runtime_error("unknown argument: " + a);
    }

    if (save_dir.empty())    throw std::runtime_error("--save_dir is required");
    if (cell_name.empty())   throw std::runtime_error("--cell_name is required");
    if (config_path.empty()) throw std::runtime_error("--config is required");
    if (!schematic.empty())
        throw std::runtime_error(
            "--schematic (DP placement) is not implemented: the Python reference "
            "stdcellgen.placement.Placer has no dynamic_programming method (dangling "
            "reference at flow/routing_flow.py:123). Use --placement_file for routing-only.");
    if (placement_file.empty())
        throw std::runtime_error("--placement_file is required (routing-only mode)");

    const std::string database_dir  = save_dir + "/" + cell_name;
    const std::string gds_path      = database_dir + "/" + cell_name + ".gds";
    const std::string unsat_marker  = database_dir + "/ROUTING_UNSAT.txt";
    const std::string log_path      = log_file.empty() ? (database_dir + "/run.log") : log_file;

    make_dirs(database_dir);

    std::cout << "[INPUT]\n"
              << "- save_dir: " << save_dir << "\n"
              << "- cell_name: " << cell_name << "\n"
              << "- schematic: \n"
              << "- config: " << config_path << "\n"
              << "- placement_file: " << placement_file << "\n"
              << "- no_cache: " << (no_cache ? "yes" : "no") << "\n"
              << "- regen_gds_only: " << (regen_gds_only ? "yes" : "no") << "\n";

    const bool cache_hit = (!no_cache) && !regen_gds_only && path_exists(gds_path) && !path_exists(unsat_marker);
    if (cache_hit) {
        std::cout << "[FLOW]\n- routing: cache_hit (existing GDS)\n"
                  << "[RESULT]\n- status: SAT (cached)\n- database_dir: " << database_dir
                  << "\n- gds_exists: yes (" << gds_path << ")\n";
        return 0;
    }

    Logger log;
    log.open(log_path);
    log.step("Loading design rules from " + config_path);

    aumedal::Config cfg;
    cfg.load(config_path);
    log.set_level(cfg.option.value("log_level", std::string("INFO")));
    aumedal::Circuit circ;
    circ.read_placement(placement_file);
    if (!ports_csv.empty()) {
        // A placement file lists nets, not ports, so read_placement has to
        // guess which are which. The caller knows, from the cell's own
        // subcircuit line, so its list replaces the guess and only real ports
        // reach the layout as a label and a pin shape.
        std::set<std::string> ports;
        std::string cur;
        for (char c : ports_csv + ",") {
            if (c == ',') { if (!cur.empty()) ports.insert(cur); cur.clear(); }
            else if (!isspace((unsigned char)c)) cur += c;
        }
        for (const std::string& q : ports)
            if (!circ.ext_pins.count(q))
                throw std::runtime_error("--ports names a net the placement does not have: " + q);
        circ.ext_pins.swap(ports);
    }
    circ.validate_against(cfg);
    log.step("Routing-only mode for " + cell_name + " (placement file + routing)");

    std::cout << "[FLOW]\n"
              << "- placement: routing_only (source=" << placement_file << ")\n"
              << "- routing: search_policy\n"
              << "- attempt_tag: input\n";

    log.step("Routing search policy + solve");
    const std::string routing_cache_path = database_dir + "/" + cell_name + ".routing.json";
    aumedal::RoutingResult r = route_cell(cfg, circ, cell_name, gds_path, routing_cache_path, regen_gds_only);

    if (r.sat) {
        if (path_exists(unsat_marker)) ::remove(unsat_marker.c_str());
        log.step("Routing SAT (top_layer=" + r.top_layer +
                 ", tolerance=" + std::to_string(r.tolerance) + "). GDS written to " + gds_path);
        std::cout << "[RESULT]\n- status: SAT\n- database_dir: " << database_dir
                  << "\n- log_file: " << log_path
                  << "\n- gds_exists: " << (path_exists(gds_path) ? "yes" : "no")
                  << " (" << gds_path << ")\n- objective: [";
        for (size_t i = 0; i < r.objective_values.size(); ++i)
            std::cout << (i ? "," : "") << r.objective_values[i];
        std::cout << "]\n";
        return 0;
    }

    if (r.undecided) {
        log.step("Routing UNDECIDED: the solver timed out before reaching a verdict. "
                 "No infeasibility marker written.");
        std::cout << "TIMEOUT\n[RESULT]\n- status: TIMEOUT\n- database_dir: " << database_dir
                  << "\n- log_file: " << log_path
                  << "\n- unsat_marker_exists: no\n";
        return 1;
    }
    std::ofstream(unsat_marker).close();
    log.step("Routing UNSAT. Marker written to " + unsat_marker);
    std::cout << "UNSAT\n[RESULT]\n- status: UNSAT\n- database_dir: " << database_dir
              << "\n- log_file: " << log_path
              << "\n- unsat_marker_exists: yes (" << unsat_marker << ")\n";
    return 1;

} catch (const std::exception& e) {
    std::cerr << "[FATAL] " << e.what() << "\n";
    return 2;
}
