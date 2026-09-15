#include "../src/gds.cpp"
#include <filesystem>
#include <sstream>
#include <unistd.h>

using namespace aumedal;
namespace fs = std::filesystem;

namespace {
void require(bool ok, const std::string& why) {
    if (!ok) throw std::runtime_error(why);
}

template<class F> void rejects(F run, const std::string& message) {
    bool caught = false;
    try { run(); }
    catch (const std::runtime_error& e) {
        caught = true;
        require(std::string(e.what()).find(message) != std::string::npos,
                "wrong rejection: " + std::string(e.what()));
    }
    require(caught, "expected rejection containing: " + message);
}

struct Capture {
    std::ostringstream out;
    std::streambuf* old = std::cerr.rdbuf(out.rdbuf());
    ~Capture() { std::cerr.rdbuf(old); }
};

struct Fixture {
    Config cfg;
    NetOrders no;
    RoutingResult route;
    std::vector<Net> nets;
    std::map<int, std::vector<Rect>> rects;
    std::vector<GdsLabel> labels;
    std::vector<Rect> pins;
    explicit Fixture(const Config& config) : cfg(config) {
        no.p_net = {"VDD", "A", "Y", "A", "VDD", "A", "Y"};
        no.n_net = no.p_net;
        no.p_fins.assign(no.p_net.size(), 0);
        no.n_fins.assign(no.n_net.size(), 0);
        route.sat = true;
    }
    void signal(const std::string& name, double x, double y) {
        Net n;
        n.name = name;
        n.is_ext_pin = true;
        nets.push_back(n);
        labels.push_back({name, x, y, 8, 25});
    }
    void apply() { apply_ihp_feedback(cfg, route, nets, no, rects, labels, pins); }
    std::vector<IRect> layer(int gl) const {
        std::vector<IRect> result;
        const auto it = rects.find(gl);
        if (it != rects.end()) for (const auto& r : it->second) result.push_back(to_irect(r));
        return result;
    }
};

bool on_grid(const Rect& r, long gx = 480, long gy = 420) {
    return std::ceil(r[0] / gx) * gx <= r[2] && std::ceil(r[1] / gy) * gy <= r[3];
}

// Independent scanline membership: opposite bridge traversals cancel.
bool polygon_contains(const std::vector<Pt>& p, double x, double y) {
    bool inside = false;
    for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++) {
        const auto& a = p[j]; const auto& b = p[i];
        if ((a[1] > y) != (b[1] > y) &&
            x < a[0] + (y - a[1]) * (b[0] - a[0]) / (b[1] - a[1])) inside = !inside;
    }
    return inside;
}

void check_union_samples(const std::vector<IRect>& rs, const std::vector<std::vector<Pt>>& polys) {
    std::vector<long> xs, ys;
    for (const auto& r : rs) { xs.insert(xs.end(), {r[0], r[2]}); ys.insert(ys.end(), {r[1], r[3]}); }
    std::sort(xs.begin(), xs.end()); std::sort(ys.begin(), ys.end());
    for (size_t i = 1; i < xs.size(); ++i) for (size_t j = 1; j < ys.size(); ++j) {
        if (xs[i] == xs[i-1] || ys[j] == ys[j-1]) continue;
        const double x = (xs[i] + xs[i-1]) / 2.0, y = (ys[j] + ys[j-1]) / 2.0;
        bool expected = false, actual = false;
        for (const auto& r : rs) expected |= r[0] < x && x < r[2] && r[1] < y && y < r[3];
        for (const auto& p : polys) actual |= polygon_contains(p, x, y);
        require(expected == actual, "serialized polygon changes rectangle-union membership");
    }
}

void check_sd_pair(const Fixture& f, const std::vector<IRect>& active) {
    const auto cuts = f.layer(6);
    require(cuts.size() >= 2, "expected at least two source/drain contacts");
    for (size_t i = 0; i < cuts.size(); ++i) {
        require(ir_covered(ir_grow(cuts[i], 70), active), "contact misses active enclosure");
        require(ir_covered(ir_grow(cuts[i], 50), f.layer(8)), "contact misses M1 enclosure");
        for (size_t j = 0; j < i; ++j)
            require(ir_gap(cuts[i], cuts[j]) >= 180, "contact spacing below 180");
    }
}

void sd_fixture(Fixture& f, bool horizontal) {
    f.rects[1] = horizontal ? std::vector<Rect>{{400, 850, 1600, 1150}}
                           : std::vector<Rect>{{850, 400, 1150, 1600}};
    f.rects[8] = f.rects[1];
    f.rects[6] = {{920, 920, 1080, 1080}};
}

struct TempDir {
    fs::path path;
    TempDir() {
        std::string pattern = (fs::temp_directory_path() / "aumedal-geometry-XXXXXX").string();
        char* p = mkdtemp(pattern.data());
        require(p != nullptr, "cannot create test directory");
        path = p;
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove(path / "test.gds", ec);
        fs::remove(path, ec);
    }
};

std::vector<unsigned char> bytes(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    require(f.is_open(), "cannot read test output");
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

std::vector<Rect> read_pin_rects(const fs::path& p) {
    const auto b = bytes(p);
    auto u16 = [&](size_t i) { return (unsigned(b.at(i)) << 8) | unsigned(b.at(i + 1)); };
    auto i32 = [&](size_t i) {
        uint32_t n = 0;
        for (size_t j = 0; j < 4; ++j) n = (n << 8) | b.at(i + j);
        return static_cast<int32_t>(n);
    };
    int layer = -1, datatype = -1;
    bool boundary = false, ended = false;
    std::vector<Rect> pins;
    for (size_t pos = 0; pos < b.size();) {
        require(pos + 4 <= b.size(), "truncated GDS record header");
        const unsigned len = u16(pos), token = u16(pos + 2);
        require(len >= 4 && pos + len <= b.size(), "truncated GDS record");
        if (token == 0x0800) { boundary = true; layer = datatype = -1; }
        if (token == 0x0c00) boundary = false;
        if (token == 0x0d02) layer = u16(pos + 4);
        if (token == 0x0e02) datatype = u16(pos + 4);
        if (token == 0x1003 && boundary && layer == 8 && datatype == 2) {
            require(len == 44, "pin is not a five-point rectangle");
            const long x0 = i32(pos + 4), y0 = i32(pos + 8);
            const long x1 = i32(pos + 20), y1 = i32(pos + 24);
            require(i32(pos + 12) == x1 && i32(pos + 16) == y0 &&
                    i32(pos + 28) == x0 && i32(pos + 32) == y1 &&
                    i32(pos + 36) == x0 && i32(pos + 40) == y0, "invalid pin outline");
            pins.push_back({double(x0), double(y0), double(x1), double(y1)});
        }
        if (token == 0x0400) { require(pos + len == b.size(), "data after ENDLIB"); ended = true; }
        pos += len;
    }
    require(ended, "GDS missing ENDLIB");
    return pins;
}
}

int main(int argc, char** argv) {
    Config cfg;
    cfg.load(argc == 2 ? argv[1] : "inputs/configs/sg13g2.json");
    if (argc == 4 && std::string(argv[1]) == "--merge-fixture") {
        try {
            std::ifstream input(argv[2]);
            require(input.is_open(), "cannot open merge fixture");
            nlohmann::json j; input >> j;
            std::map<int, std::vector<Rect>> rects;
            for (auto it = j.at("rects").begin(); it != j.at("rects").end(); ++it)
                rects[std::stoi(it.key())] = it.value().get<std::vector<Rect>>();
            std::ofstream output(argv[3], std::ios::binary);
            require(output.is_open(), "cannot open merge GDS");
            write_gds_header(output, j.at("name"), cfg);
            write_merged_boundaries(output, rects, 1.0 / cfg.gds_database_unit_nm, 189);
            record(output, GDS_ENDSTR, {}); record(output, GDS_ENDLIB, {});
            output.close(); require(!output.fail(), "cannot write merge GDS");
            return 0;
        } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    }
    if (argc == 3) {
        try {
            std::ifstream input(argv[1]);
            require(input.is_open(), "cannot open fixture JSON");
            nlohmann::json j;
            input >> j;
            require(j.is_array(), "fixture input must be an array");
            nlohmann::json results = nlohmann::json::array();
            for (const auto& entry : j) {
            Fixture f(cfg);
            const auto& j = entry;
            for (auto it = j.at("rects").begin(); it != j.at("rects").end(); ++it)
                f.rects[std::stoi(it.key())] = it.value().get<std::vector<Rect>>();
            for (const auto& l : j.at("labels"))
                f.labels.push_back({l.at("text"), l.at("x"), l.at("y"), l.at("layer"), l.value("texttype", 25)});
            for (const auto& n : j.at("ports")) {
                Net net;
                net.name = n.get<std::string>();
                net.is_ext_pin = true;
                net.is_power = net.name == cfg.power_net || net.name == cfg.ground_net;
                f.nets.push_back(net);
            }
            const long width = j.at("width_nm").get<long>(), gp = cfg.pitch.at("Gate");
            const long site = cfg.opt_long("site_width"), np = width / gp;
            require(np > 0 && site > 0 && ((np * gp + site - 1) / site) * site == width,
                    "fixture width disagrees with gate-span/site formula");
            f.no.p_net.assign(2 * np - 1, DUMMY_NET);
            f.no.n_net = f.no.p_net;
            f.no.p_fins.assign(f.no.p_net.size(), 0);
            f.no.n_fins.assign(f.no.n_net.size(), 0);
            f.apply();
            nlohmann::json out;
            out["name"] = j.at("name");
            out["rects"] = nlohmann::json::object();
            for (const auto& layer : f.rects) out["rects"][std::to_string(layer.first)] = layer.second;
            out["labels"] = nlohmann::json::array();
            for (const auto& l : f.labels)
                out["labels"].push_back({{"text", l.text}, {"x", l.x}, {"y", l.y}, {"layer", l.layer}, {"texttype", l.texttype}});
            out["pins"] = f.pins;
            results.push_back(out);
            }
            std::ofstream output(argv[2]);
            require(output.is_open(), "cannot open fixture output");
            output << results.dump() << '\n';
            output.close();
            require(!output.fail(), "cannot write fixture output");
            return 0;
        } catch (const std::exception& e) {
            std::cerr << e.what() << '\n';
            return 1;
        }
    }
    require(argc <= 2, "usage: test_ihp_geometry [config.json] or INPUT.json OUTPUT.json");
    int failed = 0, total = 0;
    auto test = [&](const char* name, auto fn) {
        ++total;
        try { fn(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { ++failed; std::cout << "FAIL " << name << ": " << e.what() << '\n'; }
    };
    test("merged_ring_one_keyhole_boundary", [&] {
        const std::vector<IRect> ring{{0,0,1000,200}, {0,800,1000,1000},
                                      {0,200,200,800}, {800,200,1000,800}};
        const auto polys = merged_polygons(ring);
        check_union_samples(ring, polys);
        require(polys.size() == 1, "one connected ring emitted as touching fractured boundaries");
        require(!polygon_contains(polys[0], 500, 500), "hole filled by serializer");
    });
    test("merged_multiple_holes_and_separate_component", [&] {
        const std::vector<IRect> rs{{0,0,1800,200}, {0,800,1800,1000},
            {0,200,200,800}, {800,200,1000,800}, {1600,200,1800,800}, {2200,0,2400,200}};
        const auto polys = merged_polygons(rs);
        check_union_samples(rs, polys);
        require(polys.size() == 2, "hole pieces not rejoined or separate island changed");
    });
    test("merge_hole_free_outline_unchanged", [&] {
        const std::vector<IRect> rs{{0,0,1000,200}, {0,200,200,1000}};
        const auto polys = merged_polygons(rs);
        require(polys == std::vector<std::vector<Pt>>{{{0,0},{1000,0},{1000,200},{200,200},{200,1000},{0,1000}}},
                "hole-free outline changed");
    });
    test("merge_membership_known_bad", [&] {
        const std::vector<IRect> ring{{0,0,1000,200}, {0,800,1000,1000},
                                      {0,200,200,800}, {800,200,1000,800}};
        rejects([&] { check_union_samples(ring, {{{0,0},{1000,0},{1000,1000},{0,1000}}}); }, "membership");
    });
    test("coverage_and_grid_known_bad", [&] {
        const IRect c{{580, 920, 740, 1080}};
        require(ir_covered(ir_grow(c, 70), {{400, 850, 1600, 1150}}), "coverage control");
        require(!ir_covered(ir_grow(c, 71), {{400, 850, 1600, 1150}}), "coverage guard accepts known-bad");
        require(!on_grid({101, 101, 261, 261}), "grid guard accepts known-bad");
        require(on_grid({400, 760, 1040, 920}), "grid happy control");
    });
    test("normal_signal_pins_and_full_rails", [&] {
        Fixture f(cfg);
        f.rects[8] = {{0, -220, 2400, 220}, {0, 3560, 2400, 4000},
                      {400, 760, 1040, 920}, {1360, 1600, 1520, 1760}};
        const auto before = f.rects;
        f.signal("A", 500, 840);
        f.signal("Y", 1440, 1680);
        Net power;
        power.name = "VDD"; power.is_ext_pin = true; power.is_power = true;
        f.nets.push_back(power);
        Net internal;
        internal.name = "internal";
        f.nets.push_back(internal);
        f.apply();
        require(f.pins.size() == 4, "wrong pin count");
        require(f.pins[0] == before.at(8)[0] && f.pins[1] == before.at(8)[1], "rails changed");
        require(f.pins[2] == before.at(8)[2] && f.pins[3] == before.at(8)[3], "successful pin moved");
        for (const auto& p : f.pins) require(ir_covered(to_irect(p), f.layer(8)), "pin outside M1");
        require(on_grid(f.pins[2]) && on_grid(f.pins[3]), "signal pin misses grid");
    });
    test("missing_signal_label_rejected", [&] {
        Fixture f(cfg); f.signal("A", 480, 840); f.labels.clear();
        rejects([&] { f.apply(); }, "A");
    });
    test("label_without_metal_rejected", [&] {
        Fixture f(cfg); f.signal("A", 480, 840);
        rejects([&] { f.apply(); }, "A");
    });
    test("offgrid_signal_rejected", [&] {
        Fixture f(cfg); f.signal("A", 180, 180);
        f.rects[8] = {{101, 101, 261, 261}, {400, 0, 2400, 3780}, {0, 400, 300, 3780}};
        rejects([&] { f.apply(); }, "routing grid");
    });
    test("feasible_pin_extension_retained", [&] {
        Fixture f(cfg); f.signal("A", 700, 840);
        f.rects[8] = {{620, 760, 780, 920}};
        f.apply();
        require(f.pins.size() == 3 && on_grid(f.pins[2]), "extension did not produce grid pin");
        require(ir_covered(to_irect(f.pins[2]), f.layer(8)), "extended pin outside M1");
    });
    test("feasible_horizontal_sd_pair", [&] {
        Fixture f(cfg); sd_fixture(f, true); const auto active = f.layer(1);
        f.apply(); check_sd_pair(f, active);
        for (const auto& c : f.layer(6)) require(c[1] == 920 && c[3] == 1080, "horizontal pair moved in y");
    });
    test("vertical_sd_result_unchanged", [&] {
        Fixture f(cfg); sd_fixture(f, false); const auto active = f.layer(1);
        f.apply(); check_sd_pair(f, active);
        require(f.layer(6) == std::vector<IRect>{{920, 750, 1080, 910}, {920, 1090, 1080, 1250}},
                "existing vertical result changed");
    });
    test("existing_sd_pair_unchanged", [&] {
        Fixture f(cfg);
        f.rects[1] = f.rects[8] = {{850, 500, 1150, 1900}};
        f.rects[6] = {{920, 920, 1080, 1080}, {920, 1260, 1080, 1420}};
        const auto before = f.rects[6];
        f.apply(); require(f.rects[6] == before, "already-paired contacts changed");
    });
    test("blocked_second_contact_nonfatal_with_summary", [&] {
        Fixture f(cfg);
        f.rects[1] = f.rects[8] = {{850, 850, 1150, 1150}};
        f.rects[6] = {{920, 920, 1080, 1080}};
        Capture log; f.apply();
        require(f.rects[6].size() == 1, "blocked contact was forced");
        require(log.out.str().find("sd_unmet=1") != std::string::npos, "missing unmet SD count");
        require(log.out.str().find("no_candidate_in_search") != std::string::npos, "missing optional reason");
    });
    test("optional_m1_enclosure_nonfatal_with_summary", [&] {
        Fixture f(cfg);
        f.rects[8] = {{920, 920, 1080, 1080}, {600, 600, 735, 1400}, {1265, 600, 1400, 1400},
                      {900, 600, 1100, 735}, {900, 1265, 1100, 1400}};
        f.rects[6] = {{920, 920, 1080, 1080}};
        Capture log; f.apply();
        require(!ir_covered({870, 870, 1130, 1130}, f.layer(8)), "blocked M1 enclosure was forced");
        require(log.out.str().find("m1_enclosure_unmet=1") != std::string::npos, "missing unmet enclosure count");
    });
    test("horizontal_sd_foreign_metal_blocked", [&] {
        Fixture f(cfg); sd_fixture(f, true);
        f.rects[8] = {{850, 850, 1150, 1150}, {300, 600, 650, 1400}, {1350, 600, 1700, 1400}};
        f.apply(); require(f.rects[6].size() == 1, "horizontal fallback crossed foreign M1");
    });
    test("horizontal_sd_gate_barrier_blocked", [&] {
        Fixture f(cfg); sd_fixture(f, true);
        f.rects[5] = {{700, 800, 800, 1200}, {1200, 800, 1300, 1200}};
        f.apply(); require(f.rects[6].size() == 1, "horizontal fallback crossed gate");
    });
    test("horizontal_centered_gate_pair", [&] {
        Fixture f(cfg);
        f.rects[5] = {{650, 850, 1350, 1150}};
        f.rects[8] = {{650, 850, 1350, 1150}, {0, 0, 470, 2000}, {1530, 0, 2000, 2000},
                      {650, 0, 1350, 670}, {650, 1330, 1350, 2000}};
        f.rects[6] = {{920, 920, 1080, 1080}};
        f.apply();
        const auto cuts = f.layer(6);
        require(cuts.size() == 2, "equivalent centered horizontal gate pair omitted");
        require(ir_gap(cuts[0], cuts[1]) >= 180, "gate pair too close");
        for (const auto& c : cuts) {
            require(c[1] == 920 && c[3] == 1080, "gate pair moved vertically");
            require(ir_covered(ir_grow(c, 70), f.layer(5)), "gate enclosure missing");
            require(ir_covered(ir_grow(c, 50), f.layer(8)), "gate M1 enclosure missing");
        }
    });
    test("existing_gate_pair_unchanged", [&] {
        Fixture f(cfg);
        f.rects[5] = f.rects[8] = {{500, 500, 1800, 1900}};
        f.rects[6] = {{920, 920, 1080, 1080}, {1260, 920, 1420, 1080}};
        const auto before = f.rects[6];
        f.apply(); require(f.rects[6] == before, "already-paired gate contacts changed");
    });
    test("existing_gate_candidate_order_unchanged", [&] {
        Fixture f(cfg);
        f.rects[5] = f.rects[8] = {{500, 500, 1800, 1900}};
        f.rects[6] = {{920, 920, 1080, 1080}};
        f.apply();
        require(f.layer(6) == std::vector<IRect>{{920, 920, 1080, 1080}, {1440, 920, 1600, 1080}},
                "successful next-finger gate candidate changed");
    });
    test("horizontal_gate_active_blocked_nonfatal", [&] {
        Fixture f(cfg);
        f.rects[1] = {{600, 850, 700, 1150}, {1300, 850, 1400, 1150}};
        f.rects[5] = {{650, 850, 1350, 1150}};
        f.rects[8] = {{650, 850, 1350, 1150}, {0, 0, 470, 2000}, {1530, 0, 2000, 2000},
                      {650, 0, 1350, 670}, {650, 1330, 1350, 2000}};
        f.rects[6] = {{920, 920, 1080, 1080}};
        Capture log; f.apply();
        require(f.rects[6].size() == 1, "horizontal gate fallback crossed Active");
        require(log.out.str().find("gate_unmet=1") != std::string::npos, "missing unmet gate count");
    });
    test("writer_required_pin_failure_preserves_existing_file", [&] {
        Fixture f(cfg); f.signal("A", 480, 840);
        TempDir temp; const auto path = temp.path / "test.gds";
        { std::ofstream out(path); out << "existing-output"; }
        const auto before = bytes(path);
        rejects([&] { write_routing_gds(path.string(), "test", cfg, f.route, {}, f.no, f.nets); }, "A");
        require(bytes(path) == before, "writer truncated output before pin rejection");
    });
    test("writer_valid_gds_pin_records", [&] {
        Fixture f(cfg); TempDir temp; const auto path = temp.path / "test.gds";
        f.signal("A", 480, 840);
        f.route.metals.push_back({{260, 840, 2}, {740, 840, 2}});
        f.route.ext_pins.push_back({"A", 400, 840, 2});
        write_routing_gds(path.string(), "test", cfg, f.route, {}, f.no, f.nets);
        const auto pins = read_pin_rects(path);
        require(pins.size() == 3, "serialized GDS pin count");
        require(pins[0] == Rect{0, -220, 2400, 220}, "serialized lower rail extent");
        require(pins[1] == Rect{0, 3560, 2400, 4000}, "serialized upper rail extent");
        require(pins[2] == Rect{340, 760, 980, 920} && on_grid(pins[2]), "serialized signal pin geometry");
    });
    test("writer_open_failure", [&] {
        Fixture f(cfg); TempDir temp;
        rejects([&] { write_routing_gds((temp.path / "absent" / "test.gds").string(), "test", cfg, f.route, {}, f.no, {}); }, "open");
    });
    test("writer_write_failure", [&] {
        Fixture f(cfg);
        require(fs::exists("/dev/full"), "test requires /dev/full");
        rejects([&] { write_routing_gds("/dev/full", "test", cfg, f.route, {}, f.no, {}); }, "write");
    });
    std::cout << "TESTS " << total << " FAILED " << failed << '\n';
    return failed ? 1 : 0;
}
