#pragma once

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
    { V::leq(a, b) } -> std::same_as<Satisfiability>;
};


template <ValueDomain V>
struct NonRelational {

    struct State {
        std::unordered_map<std::string, V> env;
        bool reachable = false;
    };

    static V eval(const State& s, const Expr& e) {
        switch (e.kind) {
        case LITERAL_EXPR:
            return V::constant(e.token.literal.value.int_value);
        case VARIABLE_EXPR:
            return s.env[std::string(e.token.lexeme)];
        case UNARY_EXPR:
            return V::unary(s, eval(s, *e.left));
        case BINARY_EXPR:
            return V::binary(s, eval(s, *e.left), eval(s, *e.right));
          break;
        }
    }

    static State assign(State s, const std::string& name, const Expr& e) {
        V value = eval(s, e);
        s.env[name] = value;
        return s;
    }



};


template <class D>
concept StateDomain = requires(D::State s, const std::string& x, const Expr& e) {

    { D::assign(s, x, e) } -> std::same_as<typename D::State>;

    // { D::eval(s, e) } -> std::same_as<typename D::State>;
    // TODO: this should return a tuple <abstract value, abstract state> (function calls with side effects)

    { D::assume(s, e) } -> std::same_as<typename D::State>;
    { D::join(s, s) } -> std::same_as<typename D::State>;
    { D::widen(s, s) } -> std::same_as<typename D::State>;

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

    static Satisfiability leq(Interval a, Interval b) {
        return SAT_UNKNOWN;
    }


};


static_assert(ValueDomain<Interval>);



