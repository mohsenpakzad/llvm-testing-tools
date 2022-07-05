cmake_minimum_required(VERSION 3.21)
project(Phase_3__Dynamic_Symbolic_Execution_on_LLVM_IR)

set(CMAKE_CXX_STANDARD 14)

add_executable(Phase_3__Dynamic_Symbolic_Execution_on_LLVM_IR DseTester.cpp Utils.h PathNavigator.h Solver.h DseTester.h)
