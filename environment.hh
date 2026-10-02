#pragma once

#include <iostream>
#include <unordered_map>
#include "parser.hh"
#include "utils.hh"

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
concept ValueDomain = requires (V a, V b, isize n, const Expr& unary, const Expr& binary) {

    { V::bottom() } -> std::same_as<V>;
    { V::top() } -> std::same_as<V>;
    { V::constant(n) } -> std::same_as<V>;
    { V::unary(unary, a) } -> std::same_as<V>;
    { V::binary(binary, a, b) } -> std::same_as<V>;
    { V::join(a, b) } -> std::same_as<V>;
    { V::widen(a, b) } -> std::same_as<V>;
    { V::dump_value(a) } -> std::same_as<std::string>;
    { V::is_bottom(a) } -> std::same_as<bool>;
    { V::is_top(a) } -> std::same_as<bool>;
    { V::leq(a, b) } -> std::same_as<bool>;
};


template <ValueDomain V>
struct NonRelational {

    struct State {
        AbstractElementKind kind;
        std::unordered_map<std::string, V> env;
        bool reachable = true;
    };

    static V eval(State& s, const Expr& e) {
        switch (e.kind) {
        case LITERAL_EXPR:
            return V::constant(e.token.literal.value.int_value);
        case VARIABLE_EXPR:
            return s.env[std::string(e.token.lexeme)];
        case UNARY_EXPR:
            return V::unary(e, eval(s, *e.left));
        case BINARY_EXPR:
            return V::binary(e, eval(s, *e.left), eval(s, *e.right));
          break;
        }
    }

    static State assign(State s, const std::string& name, const Expr& e) {
        std::cout << "called non relational assign!" << std::endl;
        assert(e.kind == LITERAL_EXPR);
        V value = eval(s, e);

        std::cout << "eval during assign: " << V::dump_value(value) << std::endl;

        if (!V::is_bottom(value)) {
            s.kind = SOME;
            s.env[name] = value;
            return s;
        }
        else {
            return bot_state();
        }

    }

    static State top_state() {
        return State {.kind = TOP, .env = {}, .reachable = true};
    }

    static State bot_state() {
        return State {.kind = BOT, .env = {}, .reachable = true};
    }

    static std::string dump_state(const State& s) {

        if (s.kind == BOT)
            return "[ BOT ]";
        if (s.kind == TOP)
            return "[ TOP ]";

        std::string res = "[\n";
        for (const auto& x : s.env) {
            res += "\t" + x.first + V::dump_value(x.second);
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

    // { D::eval(s, e) } -> std::same_as<typename D::State>;
    // TODO: this should return a tuple <abstract value, abstract state> (function calls with side effects)

    // { D::assume(s, e) } -> std::same_as<typename D::State>;
    // { D::widen(s, s) } -> std::same_as<typename D::State>;

};


struct Interval {
    AbstractElementKind kind;

    isize low;
    isize high;

    static Interval bottom() {
        return Interval {.kind = BOT, .low = 0, .high = 1};
    }

    static Interval top() {
        return Interval {.kind = TOP, .low = 0, .high = 1};
    }

    static Interval constant(isize n) {
        std::cout << "constant with value " << n << std::endl;
        return Interval {.kind = SOME, .low = n, .high = n};
    }

    static Interval unary(const Expr& unary, Interval a) {
        return top();
    }

    static Interval binary(const Expr& binary, Interval a, Interval b) {
        return top();
    }

    static Interval join(Interval a, Interval b) {
        return top();
    }

    static Interval widen(Interval a, Interval b) {
        return top();
    }

    static bool is_bottom(Interval a) {
        return a.kind == BOT;
    }

    static bool is_top(Interval a) {
        return a.kind == TOP;
    }

    static bool leq(Interval a, Interval b) {
        return (a.low >= b.low && a.high <= b.high);
    }

    static std::string dump_value(Interval a) {
        switch (a.kind) {
        case SOME:
            return "[ " + std::to_string(a.low) + ", " + std::to_string(a.high) + " ] ";
        case TOP:
            return "TOP";
        case BOT:
            return "BOT";
        }
    }

};


static_assert(ValueDomain<Interval>);
static_assert(StateDomain<NonRelational<Interval>>);



