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

#include "PathVariablesRangeAnalyzer.h"

using namespace llvm;

BasicBlock *mainBasicBlock;

LLVMContext &getGlobalContext() {
    static LLVMContext context;
    return context;
}

std::vector<BasicBlock *> generateRandomPath(BasicBlock *entryBlock);

int main(int argc, char *argv[]) {
    // Read the IR file.
    LLVMContext & context = getGlobalContext();
    SMDiagnostic err;
    std::unique_ptr<Module> M = parseIRFile(argv[1], err, context);
    if (M == nullptr) {
        fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
        return EXIT_FAILURE;
    }

    // initial mainBasicBlock
    for (auto &F: *M) {
        if (F.getName() == "main") {
            mainBasicBlock = &F.getEntryBlock();
            break;
        }
    }

    auto path = generateRandomPath(mainBasicBlock);

    // reverse path and pass it to calculateVariableRanges
    std::vector<BasicBlock *> reversedPath;
    for (auto it = path.rbegin(); it != path.rend(); it++) {
        reversedPath.push_back(*it);
    }

    auto rangeVariableMap =
            PathVariablesRangeAnalyzer(reversedPath, -20, 20)
                    .getVariablesRangeMap();

    outs() << "************** Path **************" << "\n";
    for (auto &basicBlock: path) {
        outs() << getSimpleNodeName(basicBlock) << "\n";
    }

    outs() << "======== Analysis Result =========" << "\n";

    // print data of variablesRangeMap
    for (auto &it: rangeVariableMap) {
        std::cout << it.first << ": ";
        for (auto &i: it.second) {
            std::cout << i << " ";
        }
        std::cout << "\n";
    }

    outs() << "======= Random Test Output =======" << "\n";

    for (auto &it: rangeVariableMap) {
        unsigned rnd = randomInRange(0, it.second.size() - 1);

        // cast it.second to vector<int>
        std::vector<int> v(it.second.begin(), it.second.end());

        outs() << it.first << ": " << v[rnd] << "\n";
    }

    return 0;
}

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
