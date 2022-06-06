#ifndef PHASE_1__RANDOM_TESTING_ON_LLVM_IR_UTILS_H
#define PHASE_1__RANDOM_TESTING_ON_LLVM_IR_UTILS_H

#include <cstdio>
#include <iostream>
#include <set>
#include <cstdlib>
#include <random>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int randomInRange(int startOfRange, int endOfRange) {
    //the random device that will seed the generator
    std::random_device seeder;
    //then make a mersenne twister engine
    std::mt19937 engine(seeder());
    //then the easy part... the distribution
    std::uniform_int_distribution<int> dist(startOfRange, endOfRange);
    //then just generate the integer like this:
    return dist(engine);
}

std::string getSimpleNodeName(const Value *node) {
    if (!node->getName().empty())
        return node->getName().str();
    std::string str;
    raw_string_ostream os(str);
    node->printAsOperand(os, false);
    return os.str();
}

#endif //PHASE_1__RANDOM_TESTING_ON_LLVM_IR_UTILS_H
