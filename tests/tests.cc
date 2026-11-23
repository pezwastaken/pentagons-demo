
#define CATCH_CONFIG_MAIN   // Catch generates main() for you
#include "catch.hpp"

#include "../core_inc.hh"
#include "../core_inc.cc"

TEST_CASE("interval join", "[intervals]") {
    Interval a {.kind = SOME, .low = 0, .high = 10};
    Interval b {.kind = SOME, .low = 12, .high = 20};

    auto res = Interval::join(a, b);

    REQUIRE(res.kind == SOME);
    REQUIRE(res.low == 0);
    REQUIRE(res.high == 20);
}



/*
 *
 * upper bounds
 *
 */

std::set<std::string_view> get_ub_for(const std::vector<std::string_view>& v) {
    std::set<std::string_view> res {};
    for (auto s : v) {
        res.insert(s);
    }
    return res;
}

TEST_CASE("ub top", "[strict upper bounds]") {
    SubDomain::State a = SubDomain::top_state();

    REQUIRE(a.env.empty());
    REQUIRE(a.reachable);
}

TEST_CASE("ub top_join", "[strict upper bounds]") {
    SubDomain::State a = SubDomain::top_state();
    SubDomain::State b = SubDomain::top_state();

    auto c = SubDomain::join(a, b);

    REQUIRE(c.env.empty());
    REQUIRE(c.reachable);
}

TEST_CASE("ub bottom_top_join", "[strict upper bounds]") {
    SubDomain::State a = SubDomain::bot_state();
    SubDomain::State b = SubDomain::top_state();

    auto c = SubDomain::join(a, b);

    REQUIRE(c.env.empty());
    REQUIRE(c.reachable);
}

TEST_CASE("ub bottom_bottom_join", "[strict upper bounds]") {
    SubDomain::State a = SubDomain::bot_state();
    SubDomain::State b = SubDomain::bot_state();

    auto c = SubDomain::join(a, b);

    REQUIRE(c.env.empty());
    REQUIRE(!c.reachable);
}

TEST_CASE("ub bottom_some_join", "[strict upper bounds]") {
    SubDomain::State a = SubDomain::bot_state();
    SubDomain::State b = SubDomain::top_state();

    SubDomain::Sub s {.ub = get_ub_for({"x", "y"})};
    b.env.insert(std::make_pair("a", s));

    auto c = SubDomain::join(a, b);

    REQUIRE(!c.env.empty());
    REQUIRE(c.reachable);
    REQUIRE(c.env.contains("a"));
    REQUIRE(b.env["a"].ub == c.env["a"].ub);
}


TEST_CASE("ub_some_some_join", "[strict upper bounds]") {
    SubDomain::State a = SubDomain::top_state();
    SubDomain::State b = SubDomain::top_state();

    SubDomain::Sub s1 {.ub = get_ub_for({"x", "y"})};
    a.env.insert(std::make_pair("a", s1));

    SubDomain::Sub s2 {.ub = get_ub_for({"x", "y", "z"})};
    b.env.insert(std::make_pair("a", s2));

    auto c = SubDomain::join(a, b);

    REQUIRE(!c.env.empty());
    REQUIRE(c.reachable);
    REQUIRE(c.env.contains("a"));
    REQUIRE(a.env["a"].ub == c.env["a"].ub);
}

TEST_CASE("ub_some_some_join_2", "[strict upper bounds]") {
    SubDomain::State a = SubDomain::top_state();
    SubDomain::State b = SubDomain::top_state();

    SubDomain::Sub s1_1{.ub = get_ub_for({"x", "y"})};
    SubDomain::Sub s1_2 {.ub = get_ub_for({"z"})};
    a.env.insert(std::make_pair("a", s1_1));
    a.env.insert(std::make_pair("b", s1_2));

    SubDomain::Sub s2 {.ub = get_ub_for({"x", "y", "z"})};
    b.env.insert(std::make_pair("a", s2));

    auto c = SubDomain::join(a, b);

    REQUIRE(!c.env.empty());
    REQUIRE(c.reachable);
    REQUIRE(c.env.contains("a"));
    REQUIRE(a.env["a"].ub == c.env["a"].ub);
    REQUIRE(!c.env.contains("b"));
}


