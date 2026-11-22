
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

