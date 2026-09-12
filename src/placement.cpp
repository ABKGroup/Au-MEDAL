// Builds per-column net orders from the device lists and generates
// the per-layer X/Y routing tracks.
#include "placement.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aumedal {

namespace {

bool fet_is_dummy(const Mosfet& m) { return m.name == DUMMY_NAME || m.name == DUMMY_NET; }
bool net_is_dummy(const std::string& s) { return s == DUMMY_NAME || s == DUMMY_NET; }

Mosfet dummy_fet() {
    return Mosfet{DUMMY_NAME, DUMMY_NET, DUMMY_NET, DUMMY_NET, 0};
}

template <typename T, typename Pred>
void non_dummy_bounds(const std::vector<T>& v, Pred is_dummy, long& first, long& last) {
    first = 0; last = static_cast<long>(v.size()) - 1;
    bool first_assigned = false;
    for (long i = 0; i < static_cast<long>(v.size()); ++i) {
        if (!is_dummy(v[i])) {
            if (!first_assigned) { first_assigned = true; first = i; }
            last = i;
        }
    }
}

template <typename T>
void slice_range(std::vector<T>& v, long first, long last) {
    v = std::vector<T>(v.begin() + first, v.begin() + last + 1);
}

void strip_dummy_fets(std::vector<Mosfet>& n, std::vector<Mosfet>& p) {
    long fn, ln, fp, lp;
    non_dummy_bounds(n, [](const Mosfet& m) { return fet_is_dummy(m); }, fn, ln);
    non_dummy_bounds(p, [](const Mosfet& m) { return fet_is_dummy(m); }, fp, lp);
    long first = std::min(fn, fp), last = std::max(ln, lp);
    slice_range(n, first, last);
    slice_range(p, first, last);
}

void strip_dummy_nets(std::vector<std::string>& n, std::vector<std::string>& p,
                      std::vector<long>& nf, std::vector<long>& pf) {
    long fn, ln, fp, lp;
    non_dummy_bounds(n, [](const std::string& s) { return net_is_dummy(s); }, fn, ln);
    non_dummy_bounds(p, [](const std::string& s) { return net_is_dummy(s); }, fp, lp);
    long first = std::min(fn, fp), last = std::max(ln, lp);
    slice_range(n, first, last);
    slice_range(p, first, last);
    slice_range(nf, first, last);
    slice_range(pf, first, last);
}

void flatten_fet_orders(std::vector<Mosfet>& n, std::vector<Mosfet>& p) {
    long diff = static_cast<long>(n.size()) - static_cast<long>(p.size());
    if (diff > 0)
        for (long i = 0; i < diff; ++i) p.push_back(dummy_fet());
    else
        for (long i = 0; i < -diff; ++i) n.push_back(dummy_fet());
}

void add_net_point(std::vector<std::string>& net_order, std::vector<long>& num_fins,
                   const Mosfet& fet, bool diffusion_sharing) {
    long nf = fet.nfin;
    if (net_order.empty()) {
        net_order.push_back(fet.drain);
        num_fins.push_back(nf);
    } else if (diffusion_sharing) {
        if (fet.drain != DUMMY_NET) {
            net_order.back() = fet.drain;
            num_fins.back() = std::max(nf, num_fins.back());
        }
    } else {
        net_order.push_back(DUMMY_NET);
        num_fins.push_back(0);
        net_order.push_back(fet.drain);
        num_fins.push_back(nf);
    }
    net_order.push_back(fet.gate);
    num_fins.push_back(nf);
    net_order.push_back(fet.source);
    num_fins.push_back(nf);
}

bool is_break_pattern(const std::string& prev, const std::string& cur, const std::string& next,
                      bool same_net) {
    if (net_is_dummy(prev) || cur != DUMMY_NET || net_is_dummy(next)) return false;
    return same_net ? (prev == next) : (prev != next);
}

void add_diffusion_break(std::vector<std::string>& nn, std::vector<std::string>& pn,
                         std::vector<long>& nf, std::vector<long>& pf, long diff_break) {
    if (nn.size() != pn.size()) throw std::runtime_error("Length mismatch between NFET and PFET orders.");
    long i = 2;
    while (i < static_cast<long>(nn.size()) - 2) {
        const std::string& prev_n = nn[i - 1]; const std::string& cur_n = nn[i]; const std::string& next_n = nn[i + 1];
        const std::string& prev_p = pn[i - 1]; const std::string& cur_p = pn[i]; const std::string& next_p = pn[i + 1];
        bool diff_add = is_break_pattern(prev_n, cur_n, next_n, false) ||
                        is_break_pattern(prev_p, cur_p, next_p, false);
        bool same_add = is_break_pattern(prev_n, cur_n, next_n, true) ||
                        is_break_pattern(prev_p, cur_p, next_p, true);
        if (diff_add || same_add) {
            long add_num = diff_add ? 2 * (diff_break - 1) : -2;
            for (long k = 0; k < add_num; ++k) {
                nn.insert(nn.begin() + i, DUMMY_NET);
                pn.insert(pn.begin() + i, DUMMY_NET);
                nf.insert(nf.begin() + i, 0);
                pf.insert(pf.begin() + i, 0);
            }
        }
        ++i;
    }
}

}

NetOrders compute_net_orders(const std::vector<Mosfet>& nfets_in,
                             const std::vector<Mosfet>& pfets_in,
                             long diffusion_break) {
    std::vector<Mosfet> n = nfets_in, p = pfets_in;
    strip_dummy_fets(n, p);
    flatten_fet_orders(n, p);

    NetOrders r;
    if (n.size() != p.size()) throw std::runtime_error("Mismatch in FET counts");

    for (size_t i = 0; i < n.size(); ++i) {
        const Mosfet& nfet = n[i];
        const Mosfet& pfet = p[i];
        bool n_share = false;
        if (r.n_net.size() > 1) {
            const std::string& last = r.n_net.back();
            if (last == nfet.drain || last == DUMMY_NET || nfet.drain == DUMMY_NET) n_share = true;
        }
        bool p_share = false;
        if (r.p_net.size() > 1) {
            const std::string& last = r.p_net.back();
            if (last == pfet.drain || last == DUMMY_NET || pfet.drain == DUMMY_NET) p_share = true;
        }
        bool sharing = n_share && p_share;
        add_net_point(r.n_net, r.n_fins, nfet, sharing);
        add_net_point(r.p_net, r.p_fins, pfet, sharing);
    }

    add_diffusion_break(r.n_net, r.p_net, r.n_fins, r.p_fins, diffusion_break);
    strip_dummy_nets(r.n_net, r.p_net, r.n_fins, r.p_fins);

    for (auto* v : {&r.n_net, &r.p_net}) { v->insert(v->begin(), DUMMY_NET); v->push_back(DUMMY_NET); }
    for (auto* v : {&r.n_fins, &r.p_fins}) { v->insert(v->begin(), 0); v->push_back(0); }

    if (r.p_net.size() % 2 == 0 || r.n_net.size() % 2 == 0)
        throw std::runtime_error("dbNet order lengths must be odd.");
    return r;
}

std::vector<std::vector<long>> get_x_points(const Config& cfg, long width) {
    long max_x = static_cast<long>(width * cfg.x_unit);
    int num_layers = static_cast<int>(cfg.routing_layers.size());
    int gate_idx = cfg.routing_layer_index(cfg.gate_contact_layer);
    std::vector<std::vector<long>> x_points;

    const long gate_pitch = cfg.pitch.count("Gate") ? static_cast<long>(cfg.pitch.at("Gate")) : 0;

    for (int layer = 0; layer < num_layers; ++layer) {
        long resolution = std::max(1L, cfg.x_routing_resolution[layer]);
        long n = width * resolution;
        double x_unit = cfg.x_unit / resolution;
        std::vector<long> points;
        for (long i = 0; i < n; ++i) {
            if (layer == gate_idx && (i == 0 || i == n - 1)) continue;
            long px = static_cast<long>(cfg.x_offset + i * x_unit);
            // x_unit is half the gate pitch, so every second track sits
            // between two gate columns. Gate geometry centred there leaves
            // x_unit - width("Gate") to each column, which is below the
            // single-layer gate spacing minimum for this PDK and cannot be
            // fixed by any routing choice. Keep the columns only.
            if (layer == gate_idx && gate_pitch > 0 &&
                (px - static_cast<long>(cfg.x_offset)) % gate_pitch != 0) continue;
            if (px <= max_x) points.push_back(px);
        }
        x_points.push_back(std::move(points));
    }
    return x_points;
}

namespace {
std::vector<double> np_arange(double start, double stop, double step) {
    std::vector<double> out;
    if (step == 0) return out;
    long count = static_cast<long>(std::ceil((stop - start) / step));
    for (long i = 0; i < count; ++i) out.push_back(start + i * step);
    return out;
}
}

YPoints get_y_points(const Config& cfg, bool allow_below_min_track, const NetOrders* no) {
    const int num_layers = static_cast<int>(cfg.routing_layers.size());
    const long cell_height = cfg.cell_height;
    YPoints r;
    r.y_points.assign(num_layers, {});
    r.ext_pin_y_tracks.assign(num_layers, {});
    const double y_offset = cfg.y_offset;
    const int row = 0;

    const long max_y = (row + 1) * cell_height - static_cast<long>(cfg.y_unit);
    const long min_y = row * cell_height + static_cast<long>(cfg.y_unit);
    const double mid_y = min_y + (max_y - min_y) / 2.0;
    const bool is_flip = (row % 2 == 1);
    const long gate_contact_y = static_cast<long>(mid_y + (is_flip ? -cfg.np_offset : cfg.np_offset));

    for (int layer = 0; layer < num_layers; ++layer) {
        long eff_res = std::max(1L, cfg.y_routing_resolution[layer]);
        double y_unit = cfg.y_unit / eff_res;
        for (double y : np_arange(min_y, gate_contact_y, y_unit)) {
            y += y_offset;
            if (!allow_below_min_track) { if (y_unit <= std::abs(gate_contact_y - y)) r.y_points[layer].push_back((long)y); }
            else r.y_points[layer].push_back((long)y);
        }
        for (double y : np_arange(max_y, gate_contact_y, -y_unit)) {
            y -= y_offset;
            if (!allow_below_min_track) { if (y_unit <= std::abs(gate_contact_y - y)) r.y_points[layer].push_back((long)y); }
            else r.y_points[layer].push_back((long)y);
        }
        // Every gate contact row goes on every routing layer. A via needs a
        // track on both sides of it, so a row present only on the gate layer
        // is a row no contact can ever be built on.
        for (long y : cfg.gate_contact_rows()) {
            if (is_flip) y = (long)(2 * mid_y) - y;
            if (std::find(r.y_points[layer].begin(), r.y_points[layer].end(), y) ==
                r.y_points[layer].end())
                r.y_points[layer].push_back(y);
        }
    }

    if (no != nullptr && cfg.opt_bool("bulk_planar") &&
        cfg.opt_bool("active_contact_device_rows", true)) {
        const int act = cfg.routing_layer_index(cfg.active_contact_layer);
        if (act >= 0) {
            const double y_off = (double)cfg.opt_long("active_y_offset");
            std::vector<long> extra;
            for (size_t i = 0; i < no->n_fins.size(); ++i) {
                const long w = no->n_fins[i];
                if (w > 0) extra.push_back((long)std::llround((y_off + w / 2.0) / 5.0) * 5);
            }
            for (size_t i = 0; i < no->p_fins.size(); ++i) {
                const long w = no->p_fins[i];
                if (w > 0)
                    extra.push_back((long)std::llround(((double)cell_height - y_off - w / 2.0) / 5.0) * 5);
            }
            std::sort(extra.begin(), extra.end());
            extra.erase(std::unique(extra.begin(), extra.end()), extra.end());
            for (long y : extra)
                if (std::find(r.y_points[act].begin(), r.y_points[act].end(), y) == r.y_points[act].end())
                    r.y_points[act].push_back(y);
            // A via is built from the upper layer down, so a row present only on
            // Active is never reached and the contact cannot rise off it.
            if (cfg.opt_bool("device_rows_on_m1", false)) {
                const int m1 = cfg.routing_layer_index("M1");
                if (m1 >= 0)
                    for (long y : extra)
                        if (std::find(r.y_points[m1].begin(), r.y_points[m1].end(), y) ==
                            r.y_points[m1].end())
                            r.y_points[m1].push_back(y);
            }
            // Drop the rows whose island cannot sit inside any device. They can
            // never carry a contact, and leaving them in only grows the solver.
            const double half = cfg.width("Cont") / 2.0 +
                                (double)cfg.opt_long("active_contact_enclosure");
            std::vector<std::pair<double, double>> dev;
            for (size_t i = 0; i < no->n_fins.size(); ++i)
                if (no->n_fins[i] > 0) dev.push_back({y_off, y_off + (double)no->n_fins[i]});
            for (size_t i = 0; i < no->p_fins.size(); ++i)
                if (no->p_fins[i] > 0)
                    dev.push_back({(double)cell_height - y_off - (double)no->p_fins[i],
                                   (double)cell_height - y_off});
            std::vector<long> keep;
            for (long y : r.y_points[act]) {
                if (y % cell_height == 0) { keep.push_back(y); continue; }
                for (const auto& d : dev)
                    if (d.first <= (double)y - half && (double)y + half <= d.second) {
                        keep.push_back(y);
                        break;
                    }
            }
            if (!keep.empty()) r.y_points[act] = keep;
            std::sort(r.y_points[act].begin(), r.y_points[act].end());
        }
    }

    const std::string ext_pin_layer = cfg.ext_pin_layer.back();
    const auto& ext_pin_layers = cfg.same_height_layers.at(ext_pin_layer);
    std::vector<int> ext_pin_layer_nums;
    for (auto& l : ext_pin_layers) ext_pin_layer_nums.push_back(cfg.routing_layer_index(l));
    long ext_pin_width = cfg.width(ext_pin_layer);
    long y_off_pin = std::lround((cfg.opt_long("minimum_pin_length") - ext_pin_width) / 2.0);

    if (cfg.opt_bool("add_hor_tracks_for_pin")) {
        std::vector<int> upper_nums, lower_nums;
        for (auto& l : cfg.upper_layers.at(ext_pin_layer)) { int i = cfg.routing_layer_index(l); if (i >= 0) upper_nums.push_back(i); }
        for (auto& l : cfg.lower_layers.at(ext_pin_layer)) { int i = cfg.routing_layer_index(l); if (i >= 0) lower_nums.push_back(i); }
        int gate_idx = cfg.routing_layer_index(cfg.gate_contact_layer);

        std::vector<int> candidates = ext_pin_layer_nums;
        if (cfg.opt_bool("pin_tracks_on_lower_layers")) {
            if (std::find(lower_nums.begin(), lower_nums.end(), gate_idx) != lower_nums.end()) candidates.push_back(gate_idx);
            candidates.insert(candidates.end(), lower_nums.begin(), lower_nums.end());
        }
        if (cfg.opt_bool("pin_tracks_on_upper_layers"))
            candidates.insert(candidates.end(), upper_nums.begin(), upper_nums.end());

        for (int layer : candidates) {
            r.ext_pin_y_tracks[layer].push_back(gate_contact_y - y_off_pin);
            r.ext_pin_y_tracks[layer].push_back(gate_contact_y + y_off_pin);
            r.y_points[layer].insert(r.y_points[layer].end(), r.ext_pin_y_tracks[layer].begin(), r.ext_pin_y_tracks[layer].end());
        }
    }

    int ac_idx = cfg.routing_layer_index(cfg.active_contact_layer);
    if (ac_idx >= 0 && cfg.opt_bool("active_contact_tracks_from_pin_geometry")) {
        const std::string pin_layer = cfg.ext_pin_layer.back();
        double pin_width = cfg.power_width(pin_layer);
        double pin_spacing = *cfg.spacing_s2s(pin_layer, pin_layer);
        double fin_pitch = cfg.pitch.at("fin");
        double ac_width = cfg.width(cfg.active_contact_layer);
        long nmos_fins = cfg.num_max_nmos_fins, pmos_fins = cfg.num_max_pmos_fins;
        if (nmos_fins > 0)
            r.y_points[ac_idx].push_back((long)(pin_width / 2 + pin_spacing + fin_pitch * nmos_fins - ac_width / 2));
        if (pmos_fins > 0)
            r.y_points[ac_idx].push_back((long)(cell_height - (pin_width / 2 + pin_spacing + fin_pitch * pmos_fins - ac_width / 2)));
    }

    for (int layer = 0; layer < num_layers; ++layer) {
        auto& yp = r.y_points[layer];
        std::sort(yp.begin(), yp.end()); yp.erase(std::unique(yp.begin(), yp.end()), yp.end());
        auto& ep = r.ext_pin_y_tracks[layer];
        std::sort(ep.begin(), ep.end()); ep.erase(std::unique(ep.begin(), ep.end()), ep.end());
    }
    return r;
}

}
