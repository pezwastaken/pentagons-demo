#!/bin/bash

g++ -std=c++11 -fsanitize=undefined main.cc -o a.out && ./a.out
