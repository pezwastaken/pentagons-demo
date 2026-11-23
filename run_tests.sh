#!/bin/bash

g++ -std=c++20 -fsanitize=undefined tests/tests.cc -o tests/run_tests -Wall -Wextra

echo "compiled"
./tests/run_tests
