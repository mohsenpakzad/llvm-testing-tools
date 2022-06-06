#!/bin/bash
# code to ll

clang-10 -fno-discard-value-names -emit-llvm -S -o "$1".ll "$1".c
opt-10 -dot-cfg "$1".ll
mv .main.dot "$1".dot
./allfigs2pdf
rm "$1".dot

