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

#include "GeneticSearch.h"
#include "PathVariablesRangeAnalyzer.h"

using namespace llvm;

BasicBlock *mainBasicBlock;
std::set<BasicBlock *> allBlocks;

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

    // find all blocks and store them in a set
    for (auto &F: *M) {
        for (auto &BB: F) {
            allBlocks.insert(&BB);
        }
    }

    llvm::outs() << "All blocks:" << allBlocks.size() << "\n";

    // initial mainBasicBlock
    for (auto &F: *M) {
        if (F.getName() == "main") {
            mainBasicBlock = &F.getEntryBlock();
            break;
        }
    }

    GeneticSearch geneticSearch(Chromosome::createInitialPopulation(100, 5),
                                85,
                                40,
                                20
    );
    Chromosome bestChromosome = geneticSearch.run(1000, 50);

    for (auto path: bestChromosome.getPathList()) {

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
    }

    return 0;
}


