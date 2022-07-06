#ifndef PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_DSETESTER_H
#define PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_DSETESTER_H

#include <cstdio>
#include <iostream>
#include <set>
#include <cstdlib>
#include <random>
#include <utility>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "PathNavigator.h"
#include "Solver.h"
#include "Utils.h"

using namespace llvm;

class DseTester {

    // 1. navigate random path
    // 2. filter conditions that belong to input arguments
    // 3. negate last condition
    // 4. pass it to solver and get new input arguments
    // 5. navigate new path and save paths
    // 6. do this until first duplicate path is found

public:

    BasicBlock *entryBlock;
    std::set<std::string> inputArguments;
    int minRange, maxRange;

    DseTester(BasicBlock *entryBlock, std::set<std::string> inputArguments, int minRange, int maxRange)
            : entryBlock(entryBlock), inputArguments(std::move(inputArguments)),
              minRange(minRange), maxRange(maxRange) {}

    std::vector<Path> run() {

        std::vector<Path> navigatedPaths;

        auto currentArgumentsMap = randomInitialize(
                inputArguments,
                minRange,
                maxRange
        );

        int i = 0;
        while (true) {
            if(i == 2) return navigatedPaths;
            auto pathNavigator = PathNavigator(entryBlock, currentArgumentsMap);
            pathNavigator.navigate();

            // if navigated path is already exists break
//            for (auto &path: navigatedPaths) {
//                if (path.navigatedPath == pathNavigator.getPath()) {
//                    outs() << "duplicate path found\n";
//                    return navigatedPaths;
//                }
//            }
            navigatedPaths.emplace_back(currentArgumentsMap, pathNavigator.getPath());

            auto filteredCmpInsts = filterCmpInstsBaseOnInputArgs(pathNavigator.getCmpInstsStorePath());

            auto negateOfLast = negateCmpInst(filteredCmpInsts.back());
            filteredCmpInsts.pop_back();
            filteredCmpInsts.push_back(negateOfLast);

            auto solver = Solver(filteredCmpInsts, minRange, maxRange);
            currentArgumentsMap = solver.solve();

            i++;
        }

        return navigatedPaths;
    }

    std::vector<CmpInstStorePath> filterCmpInstsBaseOnInputArgs(const std::vector<CmpInstStorePath> &cmpInstsStorePath) {
        std::vector<CmpInstStorePath> filteredCmpInsts;
        for (const auto &cmpInstStorePath: cmpInstsStorePath) {
            auto cmpInstruction = cmpInstStorePath.cmpInst;

            auto opCmp1 = cmpInstruction->getOperand(0);
            auto opCmp2 = cmpInstruction->getOperand(1);

            // Example: a == b, a != b, a < b, a > b, a <= b, a >= b
            if (isa<LoadInst>(opCmp1) && isa<LoadInst>(opCmp2)) {
                std::string opCmp1Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp1));
                std::string opCmp2Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp2));

                // if both opCmp1Name and opCmp2Name are in inputArguments
                // then add this cmpInstruction to filteredCmpInsts
                if (inputArguments.find(opCmp1Name) != inputArguments.end() &&
                    inputArguments.find(opCmp2Name) != inputArguments.end()) {
                    filteredCmpInsts.emplace_back(cmpInstruction, cmpInstStorePath.storePath);
                }
            }
                // Example: a == 5
            else if (isa<LoadInst>(opCmp1) && isa<ConstantInt>(opCmp2)) {
                std::string opCmp1Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp1));

                // if opCmp1Name is in inputArguments, then add this cmpInstruction to filteredCmpInsts
                if (inputArguments.find(opCmp1Name) != inputArguments.end()) {
                    filteredCmpInsts.emplace_back(cmpInstruction, cmpInstStorePath.storePath);
                }
            }
                // Example: 7 == b
            else if (isa<ConstantInt>(opCmp1) && isa<LoadInst>(opCmp2)) {
                std::string opCmp2Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp2));

                // if opCmp2Name is in inputArguments, then add this cmpInstruction to filteredCmpInsts
                if (inputArguments.find(opCmp2Name) != inputArguments.end()) {
                    filteredCmpInsts.emplace_back(cmpInstruction, cmpInstStorePath.storePath);
                }
            }
        }
        return filteredCmpInsts;
    }

    static CmpInstStorePath negateCmpInst(const CmpInstStorePath &cmpInstStorePath) {
        return {
                new ICmpInst(
                        cmpInstStorePath.cmpInst->getInversePredicate(),
                        cmpInstStorePath.cmpInst->getOperand(0),
                        cmpInstStorePath.cmpInst->getOperand(1)
                ),
                cmpInstStorePath.storePath
        };
    }
};

#endif //PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_DSETESTER_H
