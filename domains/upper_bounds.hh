#pragma once

#include <cmath>
#include <set>

struct SubDomain {

    struct Sub {
        std::set<std::string_view> ub;
    };

    struct State {
        std::map<std::string_view, Sub> env;
        bool reachable = true;
    };


    static State assign(const CFG& cfg, const State& s, const std::string& name, const Expr& e) {

        switch (e.kind) {
            case LITERAL_EXPR:
                return forget(s, name);
            case VARIABLE_EXPR: {

                std::string_view other = e.token.lexeme;
                // if name != e:
                if (other != name) {
                    State res = forget(s, name);
                    res.env[name] = res.env[other];

                    // every z that has y in its ub, now also has x
                    for (auto& entry : res.env) {
                        if (entry.second.ub.contains(other)) {
                            entry.second.ub.insert(name);
                        }
                    }
                    return res;
                }
                return s;
            }
            case UNARY_EXPR:
                return forget(s, name);
            case BINARY_EXPR:
                return inner_assign_with_binary(cfg, s, name, e);
            case FN_CALL:
            case INPUT_EXPR:
            default: return forget(s, name);
        }
    }

    static State top_state() {
        return State {.env = {}, .reachable = true};
    }

    static State bot_state() {
        return State {.env = {}, .reachable = false};
    }

    static std::string dump_state(const State& s) {

        if (s.reachable == false)
            return "[ BOT ]";
        if (s.reachable && s.env.empty())
            return "[ TOP ]";

        std::string res = "[\n";
        for (const auto& x : s.env) {

            res += "\t " + std::string(x.first) + " = [\n";
            for (const auto& ub : x.second.ub) {
                res += "\t\t" + std::string(ub) + ", ";
            }
            res += "\t],\n";
        }
        res += "]\n";
        return res;
    }

    static bool is_bottom(const State& s) {
        return s.reachable == false;
    }

    static bool is_top(const State& s) {
        return s.reachable && s.env.empty();
    }

    static State join(const State& s1, const State& s2) {

        if (!s1.reachable) return s2;
        if (!s2.reachable) return s1;

        State res {.env = {}, .reachable = true};

        for (const auto& [var, s1_var_subs] : s1.env) {

            if (!s2.env.contains(var)) {
                continue;
            }
            else {
                const Sub& s2_var_subs = s2.env.at(var);
                Sub joined = sub_intersect(s1_var_subs, s2_var_subs);
                res.env[var] = joined;
            }
        }
        return res;
    }

    static bool leq(const State& s1, const State& s2) {

        if (!s1.reachable) return true;
        if (!s2.reachable) return false;

        for (const auto& [var, s2_subs] : s2.env) {

            if (!s1.env.contains(var)) {
                return false;
            }
            else {
                if (!sub_leq(s1.env.at(var), s2_subs))
                    return false;
            }
        }
        return true;
    }

    static bool is_reachable(const State& s) {
        return s.reachable;
    }

    static State assume(const CFG& cfg, const State& s, const Expr& e) {

        if (e.kind != BINARY_EXPR)
            return s;


        if (e.bin_op == OP_BINARY_LT) {

            bool are_vars = (cfg[e.left].kind == VARIABLE_EXPR)
                && (cfg[e.right].kind == VARIABLE_EXPR);

            if (!are_vars)
                return s;

            // `x < y`

            std::string_view bin_left_var = cfg[e.left].token.lexeme;
            std::string_view bin_right_var = cfg[e.right].token.lexeme;

            State res = s;
            for (auto& entry : res.env) {
                if (entry.second.ub.contains(bin_left_var)) {
                    entry.second.ub.insert(bin_right_var);

                    // add all the ub of y to x
                    if (s.env.contains(bin_right_var)) {
                        for (const auto& upper_bound_of_y : s.env.at(bin_right_var).ub) {
                            entry.second.ub.insert(upper_bound_of_y);
                        }
                    }
                }
            }
            return res;
        }

        return s;
    }

    static State widening(const State& prev, const State& current) {
        return join(prev, current);
    }

private:

    static State inner_assign_with_binary(const CFG& cfg, const State& s, const std::string& name, const Expr& e) {

        const Expr& bin_left_expr = cfg[e.left];
        const Expr& bin_right_expr = cfg[e.right];

        // treat cases like `name = x + c`. Assignment with binary on the right

        State res = s;

        if (bin_left_expr.kind != VARIABLE_EXPR) {
            return forget(s, name);
        }
        if (e.right.valid() && bin_right_expr.kind != LITERAL_EXPR) {
            return forget(s, name);
        }
        if (e.bin_op != OP_BINARY_ADD) {
            return forget(s, name);
        }

        std::string_view bin_var = bin_left_expr.token.lexeme;
        bool is_same_var = name == bin_var;
        isize constant_value = bin_right_expr.token.literal.value.int_value;

        if (is_same_var && constant_value > 0) {
            res.env.erase(name);
            return res;
        }

        if (is_same_var && constant_value < 0) {
            for (auto& entry : res.env) {
                if (entry.second.ub.contains(bin_var))
                    entry.second.ub.erase(bin_var);
            }
            return res;
        }

        // name = y + c. forget the upper bounds of name, add name to upperbounds of y and
        // of every variable whose upper bounds contain y
        if (!is_same_var && constant_value > 0) {
            res = forget(s, name);
            for (auto& entry : res.env) {
                if (entry.second.ub.contains(bin_var)) {
                    entry.second.ub.insert(name);
                }
            }
            return res;
        }

        // TODO: other cases like name = y - c

        return forget(s, name);
    }

    static State forget(const State& s, std::string_view name) {
        State res = s;
        res.env.erase(name);
        for (auto& entry : res.env) {
            entry.second.ub.erase(name);
        }
        return res;
    }

    static Sub sub_intersect(const Sub& s1, const Sub& s2) {

        Sub res {.ub = {}};
        for (const auto& var : s1.ub) {
            if (s2.ub.contains(var))
                res.ub.insert(var);
        }
        return res;
    }

    static bool sub_leq(const Sub& s1, const Sub& s2) {

        for (auto var : s2.ub) {
            if (!s1.ub.contains(var))
                return false;
        }
        return true;
    }

};

static_assert(StateDomain<SubDomain>);

