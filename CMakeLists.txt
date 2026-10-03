cmake_minimum_required(VERSION 3.14)
project(octagons-demo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(a.out lexer.cc parser.cc cfg.cc main.cc)

target_compile_options(a.out PRIVATE -fsanitize=undefined)
target_link_options(a.out PRIVATE -fsanitize=undefined)
