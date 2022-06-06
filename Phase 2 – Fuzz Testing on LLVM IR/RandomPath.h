#ifndef PHASE_2__FUZZ_TESTING_ON_LLVM_IR_RANDOMPATH_H
#define PHASE_2__FUZZ_TESTING_ON_LLVM_IR_RANDOMPATH_H

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

#include "Utils.h"

std::vector<BasicBlock *> generateRandomPath(BasicBlock *entryBlock) {
    std::vector<BasicBlock *> path = {entryBlock};
    Instruction * terminatorInst = entryBlock->getTerminator();
    unsigned numberOfSuccessors = terminatorInst->getNumSuccessors();

    while (numberOfSuccessors > 0) {
        BasicBlock *successor;
        if (numberOfSuccessors == 1) {
            successor = terminatorInst->getSuccessor(0);
        } else {
            unsigned rnd = randomInRange(0, numberOfSuccessors - 1);
            successor = terminatorInst->getSuccessor(rnd);
        }
        path.push_back(successor);
        terminatorInst = successor->getTerminator();
        numberOfSuccessors = terminatorInst->getNumSuccessors();
    }
    return path;
}

#endif //PHASE_2__FUZZ_TESTING_ON_LLVM_IR_RANDOMPATH_H
