#pragma once

#include <unistd.h>
struct Pentagons {

    using Intervals = NonRelational<Interval>;
    using IntervalsState = Intervals::State;
    using SubsState = SubDomain::State;

    struct State {
        IntervalsState b;
        SubsState s;
    };

    static State assign(const CFG& cfg, State entry, std::string_view name, const Expr& e) {

        IntervalsState b_out = Intervals::assign(cfg, entry.b, name, e);
        SubsState s_out = SubDomain::assign(cfg, entry.s, name, e);
        State res = make(b_out, s_out);
        reduce(res);
        return res;
    }

    static State assume(const CFG& cfg, State entry, const Expr& e) {
        IntervalsState b_out = Intervals::assume(cfg, entry.b, e);
        SubsState s_out = SubDomain::assume(cfg, entry.s, e);
        State res = make(b_out, s_out);
        reduce(res);
        return res;
    }

    static State initial_state() {
        IntervalsState b = Intervals::initial_state();
        SubsState s = SubDomain::initial_state();
        return make(b, s);
    }

    static State top_state() {
        IntervalsState b = Intervals::top_state();
        SubsState s = SubDomain::top_state();
        return make(b, s);
    }

    static State bot_state() {
        IntervalsState b = Intervals::bot_state();
        SubsState s = SubDomain::bot_state();
        return State {.b = b, .s = s};
    }

    static std::string dump_state(const State& s) {
        std::string res = "[\n";
        res += "Intervals: " + Intervals::dump_state(s.b);
        res += "\nUpper bounds: " + SubDomain::dump_state(s.s);
        res += "\n]\n";
        return res;
    }

    static bool is_bottom(const State& s) {
        return Intervals::is_bottom(s.b) || (SubDomain::is_bottom(s.s));
    }

    static bool is_top(const State& s) {
        return s.b.kind == TOP && (s.s.kind == TOP && s.s.env.empty());
    }

    static State join(const State& s1, const State& s2) {
        IntervalsState b_join = Intervals::join(s1.b, s2.b);
        SubsState s_join = SubDomain::join(s1.s, s2.s);
        return make(b_join, s_join);
    }

    static State widening(const State& prev, const State& current) {
        if (is_bottom(prev)) return current;
        IntervalsState b_w = Intervals::widening(prev.b, current.b);
        SubsState s_w = SubDomain::widening(prev.s, current.s);
        return make(b_w, s_w);
    }

    static bool leq(const State& s1, const State& s2) {
        if (is_bottom(s1)) return true;
        return Intervals::leq(s1.b, s2.b) && SubDomain::leq(s1.s, s2.s);
    }

    static bool is_reachable(const State& s) {
        return !is_bottom(s);
    }

private:

    inline static State make(IntervalsState b, SubsState s) {
        if (Intervals::is_bottom(b) || SubDomain::is_bottom(s)) return bot_state();
        return State {.b = b, .s = s};
    }

    inline static void reduce(State& in) {

        if (is_bottom(in)) return;

        bool changed = true;
        while (changed) {
            changed = false;

            for (const auto& [x, uppers] : in.s.env) {

                if (!in.b.env.contains(x)) continue;

                // x < y, refine the intervals of both
                for (std::string_view y : uppers.ub) {

                    if (!in.b.env.contains(y)) continue;

                    Interval& r_x = in.b.env[x];
                    Interval& r_y = in.b.env[y];

                    changed |= reduce_high(r_x, saturating_sub(r_y.high, 1));
                    changed |= reduce_low(r_y, saturating_add(r_x.low, 1));

                    if (r_x.low > r_x.high || r_y.low > r_y.high) {
                        bot_state_in_place(in);
                        return;
                    }

                    assert(r_x.low <= r_y.low);
                    assert(r_x.high <= r_y.high);
                }
            }
        }
    }

    inline static bool reduce_high(Interval& i, isize cap) {
        if (i.high <= cap) return false;
        i.high = cap;
        return true;
    }

    inline static bool reduce_low(Interval& i, isize cap) {
        if (i.low >= cap) return false;
        i.low = cap;
        return true;
    }

    static void bot_state_in_place(State& in) {
        IntervalsState b = Intervals::bot_state();
        SubsState s = SubDomain::bot_state();
        in.b = b;
        in.s = s;
    }

};

static_assert(StateDomain<Pentagons>);
