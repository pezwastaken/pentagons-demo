#!/bin/bash

bear -- g++ -std=c++20 -g -fsanitize=undefined lexer.cc parser.cc main.cc cfg.cc -o a.out

./a.out
