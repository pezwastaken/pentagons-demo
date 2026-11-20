#pragma once

#include <climits>
#include <iostream>
#include <unordered_map>

enum AbstractElementKind {
    SOME,
    TOP,
    BOT,
};

enum Satisfiability {
    SAT_UNKNOWN,
    SAT_SAT,
    SAT_UNSAT,
    SAT_BOT,
};


template <class V>
concept ValueDomain = requires (V a, V b, isize n, const Expr& unary, const Expr& binary,
                                BinaryOperatorKind op) {

    { V::bottom() } -> std::same_as<V>;
    { V::top() } -> std::same_as<V>;
    { V::constant(n) } -> std::same_as<V>;
    { V::unary(unary, a) } -> std::same_as<V>;
    { V::binary(binary, a, b) } -> std::same_as<V>;
    { V::input_with_bounds(a, b) } -> std::same_as<V>;
    { V::satisfies(binary, a, b) } -> std::same_as<Satisfiability>;
    { V::join(a, b) } -> std::same_as<V>;
    { V::meet(a, b) } -> std::same_as<V>;
    { V::widen(a, b) } -> std::same_as<V>;
    { V::dump_value(a) } -> std::same_as<std::string>;
    { V::is_bottom(a) } -> std::same_as<bool>;
    { V::is_top(a) } -> std::same_as<bool>;
    { V::leq(a, b) } -> std::same_as<bool>;
    { V::filter(op, a, b) } -> std::same_as<V>;
};


template <ValueDomain V>
struct NonRelational {

    struct State {
        AbstractElementKind kind = BOT;
        std::unordered_map<std::string, V> env;
        bool reachable = true;
    };

    static V eval(State& s, const Expr& e) {
        switch (e.kind) {
        case LITERAL_EXPR:
            return V::constant(e.token.literal.value.int_value);
        case VARIABLE_EXPR:
            {
                if (s.env.find(std::string(e.token.lexeme)) == s.env.end()) {
                    std::cerr << "accessing non initialized variable `" << std::string(e.token.lexeme) << "`" << std::endl;
                    return V::bottom();
                }

                return s.env[std::string(e.token.lexeme)];
            }
        case UNARY_EXPR:
            return V::unary(e, eval(s, *e.left));
        case BINARY_EXPR:
            {
                V eval_left = eval(s, *e.left);
                V eval_right = eval(s, *e.right);
                return V::binary(e, eval_left, eval_right);
            }
        case INPUT_EXPR:
            {
                V eval_left = eval(s, *e.left);
                V eval_right = eval(s, *e.right);
                return V::input_with_bounds(eval_left, eval_right);
            }
        default:
            return V::top();
        }
    }

    static State assign(State s, const std::string& name, const Expr& e) {
        V value = eval(s, e);

        if (!V::is_bottom(value)) {
            s.kind = SOME;
            s.env[name] = value;
            return s;
        }
        else {
            return bot_state();
        }

    }

    static State set(State s, const std::string& name, V value) {
        s.kind = SOME;
        s.env[name] = value;
        return s;
    }

    static State assume(State s, const Expr& e) {

        if (e.kind != BINARY_EXPR)
            return bot_state();

        return inner_assume(s, e);
    }

    static State inner_assume(State s, const Expr& e) {

        if (e.left->kind == VARIABLE_EXPR) {
            V left_value = eval(s, *e.left);
            V right_value = eval(s, *e.right);

            V filtered = V::filter(e.bin_op, left_value, right_value);
            if (V::is_bottom(filtered)) {
                return bot_state();
            }

            return set(s, std::string(e.left->token.lexeme), filtered);
        }

        return s;
    }

    static State top_state() {
        return State {.kind = TOP, .env = {}, .reachable = true};
    }

    static State bot_state() {
        return State {.kind = BOT, .env = {}, .reachable = false};
    }

    static std::string dump_state(const State& s) {

        if (s.kind == BOT)
            return "[ BOT ]";
        if (s.kind == TOP)
            return "[ TOP ]";

        std::string res = "[\n";
        for (const auto& x : s.env) {
            res += "\t" + x.first + " = " + V::dump_value(x.second) + "\n";
        }
        res += "]\n";
        return res;
    }

    static bool is_bottom(const State& s) {
        return s.kind == BOT;
    }

    static bool is_top(const State& s) {
        return s.kind == TOP;
    }

    static State join(const State& s1, const State& s2) {

        if (s1.kind == TOP || s2.kind == TOP) {
            return top_state();
        }

        if (s1.kind == BOT)
            return s2;
        if (s2.kind == BOT)
            return s1;

        State res = s1;
        for (const auto& [name, v2] : s2.env) {

            auto it_value = s1.env.find(name);
            if (it_value != s1.env.end()) {

                V value = V::join(v2, it_value->second);
                res.env[name] = value;
            }
            else {
                res.env[name] = v2;
            }
        }
        return res;
    }

    static bool leq(const State& s1, const State& s2) {

        if (s2.kind == TOP)
            return true;

        if (s1.kind == TOP && s2.kind == TOP)
            return true;

        if (s2.kind == BOT)
            return s1.kind == BOT;

        for (const auto& [name, value] : s1.env) {
            auto it = s2.env.find(name);
            if (it == s2.env.end())
                return false;
            else {
                V value2 = it->second;
                if (!V::leq(value, value2))
                    return false;
            }
        }
        return true;
    }

    static bool is_reachable(const State& s) {
        return s.reachable;
    }

};

template <class D>
concept StateDomain = requires(D::State s, const std::string& x, const Expr& e) {

    { D::assign(s, x, e) } -> std::same_as<typename D::State>;
    { D::top_state() } -> std::same_as<typename D::State>;
    { D::bot_state() } -> std::same_as<typename D::State>;
    { D::dump_state(s) } -> std::same_as<std::string>;
    { D::is_bottom(s) } -> std::same_as<bool>;
    { D::is_top(s) } -> std::same_as<bool>;
    { D::join(s, s) } -> std::same_as<typename D::State>;
    { D::leq(s, s) } -> std::same_as<bool>;

    { D::is_reachable(s) } -> std::same_as<bool>;

    // { D::eval(s, e) } -> std::same_as<typename D::State>;
    // TODO: this should return a tuple <abstract value, abstract state> (function calls with side effects)

    { D::assume(s, e) } -> std::same_as<typename D::State>;
    // { D::widen(s, s) } -> std::same_as<typename D::State>;

};


template <StateDomain D>
struct FixpointResult {
    std::unordered_map<Node*, typename D::State> node_states;
    std::unordered_map<Node*, Satisfiability> asserts_sat;
};


