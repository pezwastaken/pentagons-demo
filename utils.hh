#pragma once

#include <climits>
#include <string>
#include <fstream>
#include <sstream>

#define TODO (assert(false))
#define UNREACHABLE (assert(false))

#define OUT

using i32 = int;
using isize = long long;
using u32 = unsigned int;
using usize = unsigned long long;

inline std::string read_file(std::string name) {
    std::ifstream t(name);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

[[gnu::pure]]
inline isize saturating_add(isize x, isize y) {
    isize res;
    if (!__builtin_add_overflow(x, y, &res))
        return res;

    if (y > 0) return LONG_LONG_MAX;
    return LONG_LONG_MIN;
}

[[gnu::pure]]
inline isize saturating_sub(isize x, isize y) {
    isize res;
    if (!__builtin_sub_overflow(x, y, &res))
        return res;

    if (y > 0) return LONG_LONG_MIN;
    return LONG_LONG_MAX;
}

