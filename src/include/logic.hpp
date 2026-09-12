// Cond: lazy boolean expression helpers over z3 (And/Or/Not/Equal).
#pragma once
#include <optional>
#include <vector>
#include "z3++.h"

namespace aumedal {
namespace logic {

struct Cond {
    enum Kind { NONE, TRUE_, FALSE_, EXPR } kind = NONE;
    std::optional<z3::expr> e;

    static Cond None() { return Cond{NONE, std::nullopt}; }
    static Cond True() { return Cond{TRUE_, std::nullopt}; }
    static Cond False() { return Cond{FALSE_, std::nullopt}; }
    static Cond Expr(z3::expr x) { return Cond{EXPR, std::move(x)}; }
    static Cond Bool(bool b) { return b ? True() : False(); }
};

inline z3::expr mk_and(z3::context& ctx, const std::vector<z3::expr>& es) {
    z3::expr_vector v(ctx);
    for (const auto& e : es) v.push_back(e);
    return z3::mk_and(v);
}
inline z3::expr mk_or(z3::context& ctx, const std::vector<z3::expr>& es) {
    z3::expr_vector v(ctx);
    for (const auto& e : es) v.push_back(e);
    return z3::mk_or(v);
}

inline Cond And(z3::context& ctx, const std::vector<Cond>& conds,
                std::optional<int> must_match_num = std::nullopt, bool def = false) {
    std::vector<z3::expr> valid;
    int count_valid = 0;
    for (const auto& c : conds) {
        if (c.kind == Cond::NONE) continue;
        ++count_valid;
        if (c.kind == Cond::FALSE_) return Cond::False();
        if (c.kind != Cond::TRUE_) valid.push_back(*c.e);
    }
    if (count_valid == 0) return Cond::Bool(def);
    if (must_match_num.has_value() && *must_match_num != count_valid) return Cond::False();
    if (valid.empty()) return Cond::True();
    if (valid.size() == 1) return Cond::Expr(valid[0]);
    return Cond::Expr(mk_and(ctx, valid));
}

inline Cond Or(z3::context& ctx, const std::vector<Cond>& conds, bool def = false) {
    std::vector<z3::expr> kept;
    for (const auto& c : conds) {
        if (c.kind == Cond::NONE) continue;
        if (c.kind == Cond::TRUE_) return Cond::True();
        if (c.kind != Cond::FALSE_) kept.push_back(*c.e);
    }
    if (kept.empty()) return Cond::Bool(def);
    if (kept.size() == 1) return Cond::Expr(kept[0]);
    return Cond::Expr(mk_or(ctx, kept));
}

inline Cond Not(const Cond& c, bool def = true) {
    if (c.kind == Cond::TRUE_) return Cond::False();
    if (c.kind == Cond::FALSE_) return Cond::True();
    if (c.kind == Cond::NONE) return Cond::Bool(def);
    return Cond::Expr(!*c.e);
}

inline Cond Equal(z3::context& ctx, const Cond& a, const Cond& b) {
    Cond t1 = Or(ctx, {Not(a), b});
    Cond t2 = Or(ctx, {Not(b), a});
    return And(ctx, {t1, t2});
}

inline z3::expr eq_expr(z3::context& ctx, const z3::expr& var, const Cond& c) {
    switch (c.kind) {
        case Cond::EXPR: return var == *c.e;
        case Cond::TRUE_: return var == ctx.bool_val(true);
        case Cond::FALSE_: return var == ctx.bool_val(false);
        default: return var == ctx.bool_val(false);
    }
}

}
}
