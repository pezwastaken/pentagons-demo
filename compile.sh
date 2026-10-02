#!/bin/bash

bear -- g++ -std=c++20 -g -fsanitize=undefined lexer.cc parser.cc cfg.cc main.cc -o a.out && ./a.out
