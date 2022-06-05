#!/bin/bash
# compile and run

clang++-10  -o RandomPath RandomPath.cpp `llvm-config-10 --cxxflags` `llvm-config-10 --ldflags` `llvm-config-10 --libs` -lpthread -lncurses -ldl
 ./RandomPath "$1"