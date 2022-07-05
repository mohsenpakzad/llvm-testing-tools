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

#include "PathNavigator.h"
#include "Solver.h"
#include "DseTester.h"

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

    std::set<BasicBlock *> allBlocks;
    // find all blocks and store them in a set
    for (auto &F: *M) {
        for (auto &BB: F) {
            allBlocks.insert(&BB);
        }
    }

    auto dseTester = DseTester(
            mainBasicBlock,
            getInputArguments(mainBasicBlock, "a"),
            -100,
            100
    );

    auto navigatedPaths = dseTester.run();
    std::set<BasicBlock *> navigatedBlocks;

    for (auto &path: navigatedPaths) {
        outs() << "************** Input Argument(s) ***************" << "\n";
        for (auto &arg: path.argumentsMap) {
            outs() << arg.first << " = " << arg.second << "\n";
        }

        outs() << "*************** Navigated Path *****************" << "\n";
        for (auto &basicBlock: path.navigatedPath) {
            outs() << getSimpleNodeName(basicBlock) << "\n";
            navigatedBlocks.insert(basicBlock);
        }
    }

    outs() << "****************** Coverage ********************" << "\n";
    outs() << (int)((float) navigatedBlocks.size() / allBlocks.size() * 100) << "%\n";


//    {
//        outs() << "&&&&&&&&&&&&&&& test1 &&&&&&&&&&&&" << "\n";
//        std::map<std::string, int> currentArgumentsMap = {
//                {"a1", 90}
//        };
////        outs() << "entryBlock: " << mainBasicBlock << "\n";
////        outs() << "entryBlock value: " << *mainBasicBlock << "\n";
//
//        auto pathNavigator1 = PathNavigator(mainBasicBlock, currentArgumentsMap);
//        pathNavigator1.navigate();
//        for (auto &basicBlock: pathNavigator1.getPath()) {
//            outs() << getSimpleNodeName(basicBlock) << "\n";
//        }
//    }
//
//    {
//        outs() << "&&&&&&&&&&&&&&& test2 &&&&&&&&&&&&" << "\n";
//        std::map<std::string, int> currentArgumentsMap = {
//                {"a1", 90}
//        };
////        outs() << "entryBlock: " << mainBasicBlock << "\n";
////        outs() << "entryBlock value: " << *mainBasicBlock << "\n";
//
//        auto pathNavigator1 = PathNavigator(mainBasicBlock, currentArgumentsMap);
//        pathNavigator1.navigate();
//        for (auto &basicBlock: pathNavigator1.getPath()) {
//            outs() << getSimpleNodeName(basicBlock) << "\n";
//        }
//    }



//    auto initialArgumentsMap = randomInitialize(
//            getInputArguments(mainBasicBlock, "a"),
//            -100,
//            100
//    );
//    initialArgumentsMap["a1"] = 90;
//
//    auto pathNavigator = PathNavigator(mainBasicBlock, initialArgumentsMap);
//    pathNavigator.navigate();
//
//    outs() << "************** Input Argument(s) ***************" << "\n";
//    for (auto &argument: initialArgumentsMap) {
//        outs() << argument.first << ": " << argument.second << "\n";
//    }
//
//    outs() << "*************** Navigated Path *****************" << "\n";
//    for (auto basicBlock: pathNavigator.getPath()) {
//        outs() << getSimpleNodeName(basicBlock) << "\n";
//    }
//
//    outs() << "**************** Variables Map *****************" << "\n";
//    for (auto &variable: pathNavigator.getVariablesMap()) {
//        outs() << variable.first << ": " << variable.second << "\n";
//    }
//
//    outs() << "*********** Comparison Instructions ************" << "\n";
//    for (auto &cmpInstruction: pathNavigator.getCmpInstsWithStores()) {
//        outs() << CmpInstructionToString(cmpInstruction) << "\n";
//    }

//    auto solver = Solver(pathNavigator.getCmpInstsWithStores(), -10, 10);
//    solver.applyComparisons();
//
//    // print solver variablesRange
//    outs() << "************** Variables Range *******************" << "\n";
//    for (auto &variable: solver.variablesRange) {
//        outs() << variable.first << ": ";
//        for (auto &e: variable.second) {
//            outs() << e << " ";
//        }
//        outs() << "\n";
//    }


    return 0;
}
