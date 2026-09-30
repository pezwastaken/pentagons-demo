#pragma once

#include <string>
#include <fstream>
#include <sstream>

#define TODO (exit(1))
#define UNREACHABLE (assert(false))

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


