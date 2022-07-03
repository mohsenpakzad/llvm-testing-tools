#!/bin/bash
# compile and run

clang++-10  -o DseTester DseTester.cpp `llvm-config-10 --cxxflags` `llvm-config-10 --ldflags` `llvm-config-10 --libs` -lpthread -lncurses -ldl -fexceptions
 ./DseTester "$1"