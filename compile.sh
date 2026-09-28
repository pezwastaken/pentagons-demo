#!/bin/bash

g++ -fsanitize=undefined main.cc -o a.out && ./a.out
