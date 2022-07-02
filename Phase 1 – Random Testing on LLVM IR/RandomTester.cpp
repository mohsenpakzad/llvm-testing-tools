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
#include "PathNavigator.h"

using namespace llvm;

LLVMContext &getGlobalContext() {
    static LLVMContext context;
    return context;
}


int main(int argc, char *argv[]) {
    // Read the IR file.
    LLVMContext & context = getGlobalContext();
    SMDiagnostic err;
    std::unique_ptr<Module> M = parseIRFile(argv[1], err, context);
    if (M == nullptr) {
        fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
        return EXIT_FAILURE;
    }

    BasicBlock *mainBasicBlock;
    // initial mainBasicBlock
    for (auto &F: *M) {
        if (F.getName() == "main") {
            mainBasicBlock = &F.getEntryBlock();
            break;
        }
    }

    auto argumentsMap = randomInitialize(
            getInputArguments(mainBasicBlock, "a"),
            -100,
            100
    );

    auto pathNavigator = PathNavigator(mainBasicBlock, argumentsMap);
    pathNavigator.navigate();

    outs() << "************** Input Argument(s) ***************" << "\n";
    for (auto &argument: argumentsMap) {
        outs() << argument.first << ": " << argument.second << "\n";
    }

    outs() << "*************** Navigated Path *****************" << "\n";
    for (auto basicBlock: pathNavigator.getPath()) {
        outs() << getSimpleNodeName(basicBlock) << "\n";
    }

    outs() << "**************** Variables Map *****************" << "\n";
    for (auto &variable: pathNavigator.getVariablesMap()) {
        outs() << variable.first << ": " << variable.second << "\n";
    }

    outs() << "*********** Comparison Instructions ************" << "\n";
    for (auto &cmpInstruction: pathNavigator.getCmpInstructions()) {
        outs() << CmpInstructionToString(cmpInstruction) << "\n";
    }

    return 0;
}
