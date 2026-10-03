#!/bin/bash

# bear -- g++ -std=c++20 -g -fsanitize=undefined lexer.cc parser.cc cfg.cc main.cc -o a.out && ./a.out

cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cp build/compile_commands.json compile_commands.json

cmake --build build

./build/a.out

