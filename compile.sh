#!/bin/bash

g++ -std=c++20 -g -fsanitize=undefined main.cc -o a.out -Wall -Wextra

./a.out

