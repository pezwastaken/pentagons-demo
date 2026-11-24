#!/bin/bash

ASAN_OPTIONS=detect_stack_use_after_return=1 g++ -std=c++20 -g -fsanitize=undefined -fsanitize=address main.cc -o a.out -Wall -Wextra

./a.out

