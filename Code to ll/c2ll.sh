#!/bin/bash
# code to ll

clang-10 -fno-discard-value-names -emit-llvm -S -o test1.ll test1.c
opt-10 -dot-cfg test1.ll
mv .main.dot test1.dot
./allfigs2pdf
