// flow: the routing-only cell-generation CLI.
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
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

// Content identity for local cache invalidation, not an authenticity signature.
std::string content_id(std::istream& in) {
    std::uint64_t hash = UINT64_C(14695981039346656037), size = 0;
    char buf[8192];
    while (in) {
        in.read(buf, sizeof buf);
        const auto n = in.gcount();
        size += n;
        for (std::streamsize i = 0; i < n; ++i) {
            hash ^= static_cast<unsigned char>(buf[i]);
            hash *= UINT64_C(1099511628211);
        }
    }
    if (in.bad() || !in.eof()) throw std::runtime_error("failed to read cache identity");
    std::ostringstream out;
    out << "fnv1a64:" << std::hex << hash << ':' << std::dec << size;
    return out.str();
}

std::string json_id(const nlohmann::json& value) {
    std::istringstream in(value.dump());
    return content_id(in);
}

std::string file_id(const std::string& path) {
    struct stat st;
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size == 0)
        throw std::runtime_error("cache identity requires a nonempty regular file: " + path);
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read cache identity: " + path);
    return content_id(in);
}

nlohmann::json input_identity(const aumedal::Config& cfg, const aumedal::Circuit& circ,
                              const std::string& cell_name) {
    // Fingerprint the values actually parsed, so whitespace and port order do not matter.
    auto devices = [](const std::vector<aumedal::Mosfet>& fets) {
        nlohmann::json rows = nlohmann::json::array();
        for (const auto& f : fets) rows.push_back({f.name, f.drain, f.gate, f.source, f.nfin});
        return rows;
    };
    return {{"cell_name", cell_name}, {"ports", circ.ext_pins},
            {"config", json_id({cfg.rules, cfg.specs, cfg.option})},
            {"placement", json_id({devices(circ.nfets), devices(circ.pfets)})}};
}

nlohmann::json read_cache(const std::string& path) {
    std::ifstream in(path);
    if (!in) return nlohmann::json::object();
    try {
        nlohmann::json cached;
        in >> cached;
        return cached;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[CACHE] Ignoring unreadable routing cache: " << e.what() << '\n';
        return nlohmann::json::object();
    }
}

bool matching_route(nlohmann::json cached, const nlohmann::json& inputs) {
    try {
        const auto meta = cached.at("_cache");
        cached.erase("_cache");
        return meta.at("version") == 1 && meta.at("inputs") == inputs &&
               !meta.at("solver").get<std::string>().empty() &&
               cached.at("sat") == true && !cached.value("geometry_unresolved", false) &&
               meta.at("routing") == json_id(cached);
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

void publish_cache(const std::string& path, const nlohmann::json& cached) {
    const std::string pending = path + ".tmp";
    std::ofstream out;
    out.exceptions(std::ios::failbit | std::ios::badbit);
    out.open(pending);
    out << cached.dump(2);
    out.close();
    if (::rename(pending.c_str(), path.c_str()) != 0)
        throw std::runtime_error("cannot publish routing cache: " + path);
}

void invalidate_gds_certificate(const std::string& path, nlohmann::json cached) {
    if (!cached.is_object() || !cached.contains("_cache") || !cached.at("_cache").is_object()) return;
    // Preserve the expensive solved route for an explicit retry after emitter failure.
    cached["_cache"].erase("gds");
    cached["_cache"].erase("emitter");
    publish_cache(path, cached);
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
                                  const std::string& routing_cache_path, bool regen_gds_only,
                                  const nlohmann::json& cached, const nlohmann::json& inputs,
                                  const std::string& producer) {
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
        r = aumedal::routing_result_from_json(cached);
    }

    // A failed fresh solve or write must not leave an earlier GDS certified.
    invalidate_gds_certificate(routing_cache_path, cached);
    if (!regen_gds_only) r = aumedal::solve_router(cfg, circ, no, smt2_default);

    if (r.sat && !gds_out.empty()) {
        aumedal::write_routing_gds(gds_out, cell_name, cfg, r, pre, no, cnets);
        auto result = aumedal::routing_result_to_json(r);
        // Replay may use a rebuilt emitter; it does not certify a solve by the new binary.
        const std::string solver = regen_gds_only
            ? cached.at("_cache").at("solver").get<std::string>() : producer;
        result["_cache"] = {{"version", 1}, {"inputs", inputs}, {"solver", solver},
                            {"emitter", producer}, {"routing", json_id(result)},
                            {"gds", file_id(gds_out)}};
        publish_cache(routing_cache_path, result);
    }
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
    const std::string routing_cache_path = database_dir + "/" + cell_name + ".routing.json";
    const auto inputs = input_identity(cfg, circ, cell_name);
    // Linux cache identity uses the running executable rather than argv[0].
    const std::string producer = file_id("/proc/self/exe");
    const auto cached = read_cache(routing_cache_path);
    const bool reusable = matching_route(cached, inputs);
    if (regen_gds_only && !reusable)
        throw std::runtime_error("--regen_gds_only: cached routing is missing, unvalidated or incompatible "
                                 "with config, placement or ports; run without --regen_gds_only first");
    if (!no_cache && !regen_gds_only && reusable && !path_exists(unsat_marker)) {
        bool hit = false;
        try {
            const auto& meta = cached.at("_cache");
            hit = meta.at("solver") == producer && meta.at("emitter") == producer &&
                  meta.at("gds") == file_id(gds_path);
        } catch (const std::exception& e) {
            std::cerr << "[CACHE] Ignoring unmatched GDS: " << e.what() << '\n';
        }
        if (hit) {
            std::cout << "[FLOW]\n- routing: cache_hit (matching inputs, producer and GDS)\n"
                      << "[RESULT]\n- status: SAT (cached)\n- database_dir: " << database_dir
                      << "\n- gds_exists: yes (" << gds_path << ")\n";
            return 0;
        }
    }
    log.step("Routing-only mode for " + cell_name + " (placement file + routing)");

    std::cout << "[FLOW]\n"
              << "- placement: routing_only (source=" << placement_file << ")\n"
              << "- routing: search_policy\n"
              << "- attempt_tag: input\n";

    log.step("Routing search policy + solve");
    aumedal::RoutingResult r = route_cell(cfg, circ, cell_name, gds_path, routing_cache_path,
                                        regen_gds_only, cached, inputs, producer);

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
        if (::remove(unsat_marker.c_str()) != 0 && errno != ENOENT)
            throw std::runtime_error("cannot clear stale infeasibility marker: " + unsat_marker);
        const std::string status = r.geometry_unresolved ? "UNDECIDED" : "TIMEOUT";
        const std::string reason = r.geometry_unresolved
            ? "geometry conflicts remain after refinement search exhaustion"
            : "the solver timed out before reaching a verdict";
        log.step("Routing " + status + ": " + reason + ". No infeasibility marker written.");
        std::cout << status << "\n[RESULT]\n- status: " << status << "\n- reason: " << reason
                  << "\n- database_dir: " << database_dir
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
