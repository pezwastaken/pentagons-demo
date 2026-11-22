#pragma once

#include <cmath>
#include <algorithm>

#define CHECK_TOP_BOT \
if (a.kind == TOP || b.kind == TOP) \
    return top(); \
if (a.kind == BOT) \
    return b; \
if (b.kind == BOT) \
    return a


struct Interval {
    AbstractElementKind kind;

    isize low;
    isize high;

    static Interval bottom() {
        return Interval {.kind = BOT, .low = 0, .high = -1};
    }

    static Interval top() {
        return Interval {.kind = TOP, .low = LONG_MIN, .high = LONG_MAX};
    }

    static Interval constant(isize n) {
        return Interval {.kind = SOME, .low = n, .high = n};
    }

    static Interval unary(const Expr& unary, Interval a) {
        switch (unary.un_op) {
        case OP_UNARY_NEG:
            return interval_negate(a);
        default:
            return bottom();
        }
    }

    static Interval binary(const Expr& binary, Interval a, Interval b) {
        switch (binary.bin_op) {

        case OP_BINARY_ADD:
            return interval_sum(a, b);
        case OP_BINARY_DIFF:
            return interval_diff(a, b);
        case OP_BINARY_MUL:
            return interval_mul(a, b);
        case OP_BINARY_DIV:
        case OP_BINARY_EQ:
        case OP_BINARY_NE:
        case OP_BINARY_LT:
        case OP_BINARY_LE:
        case OP_BINARY_GT:
        case OP_BINARY_GE:
        case OP_BINARY_LOG_AND:
        case OP_BINARY_LOG_OR:
          break;
        }

        return top();
    }

    static Interval input_with_bounds(Interval a, Interval b) {
        // assuming they are always constants, this breaks otherwise
        assert (a.low <= b.low);
        return Interval {.kind = SOME, .low = a.low, .high = b.high};
    }

    static Satisfiability satisfies(const Expr& binary, Interval a, Interval b) {
        switch (binary.bin_op) {

        case OP_BINARY_EQ:
            return interval_eq(a, b);
        case OP_BINARY_NE:
            return interval_ne(a, b);
        case OP_BINARY_LT:
            return interval_lt(a, b);
        case OP_BINARY_LE:
            return interval_le(a, b);
        case OP_BINARY_GT:
            return interval_gt(a, b);
        case OP_BINARY_GE:
            return interval_ge(a, b);
        default:
            return Satisfiability::SAT_UNKNOWN;
        }
    }


    static Interval join(Interval a, Interval b) {
        if (a.kind == TOP || b.kind == TOP)
            return top();

        if (a.kind == BOT)
            return b;
        if (b.kind == BOT)
            return a;

        return Interval {.kind = SOME, .low = std::min(a.low, b.low),
            .high = std::max(a.high, b.high)};
    }

    static Interval meet(Interval a, Interval b) {
        if (a.kind == BOT || b.kind == BOT)
            return bottom();

        if (a.kind == TOP)
            return b;
        if (b.kind == TOP)
            return a;

        isize l = std::max(a.low, b.low);
        isize h = std::min(a.high, b.high);
        if (l > h)
            return bottom();

        return Interval {.kind = SOME, .low = l, .high = h};
    }

    static Interval widening(Interval a, Interval b) {
        (void) a;
        (void) b;
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

    static Interval filter(BinaryOperatorKind op, Interval a, Interval b) {
        switch (op) {
        case OP_BINARY_EQ: {
            Interval glb = Interval::meet(a, b);
            return glb;
        }
        case OP_BINARY_NE: {
            if (b.kind == TOP)
                return top();
            Interval glb = Interval::meet(a, b);
            if (Interval::equal_check(a, glb))
                return bottom();
            return a;
        }
        case OP_BINARY_LT: {
            if (b.kind == TOP)
                return top();

            Interval tmp {.kind = SOME, .low = a.low, .high = b.high - 1};
            if (!valid(tmp))
                return bottom();
            return meet(a, tmp);
        }
        case OP_BINARY_LE: {
            if (b.kind == TOP)
                return top();

            Interval tmp {.kind = SOME, .low = a.low, .high = b.high};
            if (!valid(tmp))
                return bottom();
            return meet(a, tmp);
        }
        case OP_BINARY_GT: {
            if (b.kind == TOP)
                return top();

            Interval tmp {.kind = SOME, .low = b.low + 1, .high = a.high};
            if (!valid(tmp))
                return bottom();

            return meet(a, tmp);
        }
        case OP_BINARY_GE: {
            if (b.kind == TOP)
                return top();

            Interval tmp {.kind = SOME, .low = b.low, .high = a.high};
            if (!valid(tmp))
                return bottom();

            return meet(a, tmp);
        }
        case OP_BINARY_LOG_AND:
        case OP_BINARY_LOG_OR:
        default: return a;
        }
    }

    static std::string dump_value(Interval a) {
        switch (a.kind) {
        case SOME:
            return "[ " + std::to_string(a.low) + ", " + std::to_string(a.high) + " ] ";
        case TOP:
            return "TOP";
        case BOT:
            return "BOT";
        default:
            UNREACHABLE;
            return "unknown value";
        }
    }

private:

    static Interval interval_sum(Interval a, Interval b) {
        CHECK_TOP_BOT;
        return Interval {.kind = SOME, .low = a.low + b.low, .high = a.high + b.high};
    }

    static Interval interval_diff(Interval a, Interval b) {
        CHECK_TOP_BOT;
        return Interval {.kind = SOME, .low = a.low - b.high, .high = a.high + b.low};
    }

    static Interval interval_mul(Interval a, Interval b) {
        CHECK_TOP_BOT;
        isize l = std::min({ a.low * b.low,
            a.low * b.high,
            a.high * b.low,
            a.high * b.high });
        isize h = std::max({ a.low * b.low,
            a.low * b.high,
            a.high * b.low,
            a.high * b.high });
        return Interval {.kind = SOME, .low = l, .high = h };
    }

    static Interval interval_negate(Interval a) {
        if (a.kind == TOP || a.kind == BOT)
            return a;
        return Interval {.kind = SOME, .low = -a.high, .high = -a.low };
    }

    static Satisfiability interval_eq(Interval a, Interval b) {
        if (is_bottom(a) || is_bottom(b))
            return Satisfiability::SAT_BOT;

        Interval glb = meet(a, b);
        if (is_bottom(glb))
            return Satisfiability::SAT_UNSAT;
        if (is_top(glb))
            return Satisfiability::SAT_UNKNOWN;
        if (equal_check(glb, b))
            return Satisfiability::SAT_SAT;

        return Satisfiability::SAT_UNKNOWN;
    }

    static Satisfiability interval_ne(Interval a, Interval b) {
        if (is_bottom(a) || is_bottom(b))
            return Satisfiability::SAT_BOT;

        Interval glb = meet(a, b);
        if (is_bottom(glb))
            return Satisfiability::SAT_SAT;
        if (is_top(glb))
            return Satisfiability::SAT_UNKNOWN;
        if (equal_check(glb, b))
            return Satisfiability::SAT_UNSAT;

        return Satisfiability::SAT_UNKNOWN;
    }

    static Satisfiability interval_le(Interval a, Interval b) {
        if (is_bottom(a) || is_bottom(b))
            return Satisfiability::SAT_BOT;

        Interval glb = meet(a, b);
        if (is_bottom(glb))
            return Satisfiability::SAT_UNSAT;
        if (is_top(glb))
            return Satisfiability::SAT_UNKNOWN;

        if (equal_check(glb, b))
            return Satisfiability::SAT_SAT;

        return Satisfiability::SAT_UNKNOWN;
    }

    static Satisfiability interval_lt(Interval a, Interval b) {
        if (is_bottom(a) || is_bottom(b))
            return Satisfiability::SAT_BOT;

        if (a.high < b.low) {
            return Satisfiability::SAT_SAT;
        }

        if (a.low > b.high) {
            return Satisfiability::SAT_UNSAT;
        }

        return Satisfiability::SAT_UNKNOWN;
    }

    static Satisfiability interval_gt(Interval a, Interval b) {
        return sat_negate(interval_le(a, b));
    }

    static Satisfiability interval_ge(Interval a, Interval b) {
        return sat_negate(interval_lt(a, b));
    }

    static bool equal_check(Interval a, Interval b) {
        return a.low == b.low && a.high == b.high;
    }

    static bool valid(Interval a) {
        return a.low <= a.high;
    }
};

static_assert(ValueDomain<Interval>);
static_assert(StateDomain<NonRelational<Interval>>);


