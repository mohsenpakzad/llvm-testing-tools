#ifndef PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_UTILS_H
#define PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_UTILS_H

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

class Path {
public:
    std::map<std::string, int> argumentsMap;
    std::vector<BasicBlock *> navigatedPath;

    Path(std::map<std::string, int> argumentsMap, std::vector<BasicBlock *> path)
            : argumentsMap(std::move(argumentsMap)), navigatedPath(std::move(path)) {}
};

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

std::set<std::string> getInputArguments(BasicBlock *entryBlock, const std::string &prefix) {
    // get all variables that their variable name starts with "a"
    std::set<std::string> inputArguments;
    for (auto &I: *entryBlock) {
        if (I.getOpcode() == Instruction::Alloca) {
            auto allocaInstruction = dyn_cast<AllocaInst>(&I);
            if (allocaInstruction->getName().startswith(prefix)) {
                inputArguments.insert(allocaInstruction->getName());
            }
        }
    }
    return inputArguments;
}

std::map<std::string, int> randomInitialize(std::set<std::string> inputArguments, int minRange, int maxRange) {
    std::map<std::string, int> variableMap;
    for (auto &variable: inputArguments) {
        variableMap[variable] = randomInRange(minRange, maxRange);
    }
    return variableMap;
}

std::string cmpPredicateToString(CmpInst::Predicate predicate) {
    switch (predicate) {
        case CmpInst::ICMP_EQ:
            return "==";
        case CmpInst::ICMP_NE:
            return "!=";
        case CmpInst::ICMP_UGT:
            return ">";
        case CmpInst::ICMP_UGE:
            return ">=";
        case CmpInst::ICMP_ULT:
            return "<";
        case CmpInst::ICMP_ULE:
            return "<=";
        case CmpInst::ICMP_SGT:
            return ">";
        case CmpInst::ICMP_SGE:
            return ">=";
        case CmpInst::ICMP_SLT:
            return "<";
        case CmpInst::ICMP_SLE:
            return "<=";
        default:
            return "==";
    }
}

inline std::string getLoadInstOperandName(LoadInst *loadInst) {
    return loadInst->getPointerOperand()->getName().str();
}

CmpInst::Predicate negateCmpPredicate(CmpInst::Predicate predicate) {
    switch (predicate) {
        case CmpInst::ICMP_EQ:
            return CmpInst::ICMP_NE;
        case CmpInst::ICMP_NE:
            return CmpInst::ICMP_EQ;
        case CmpInst::ICMP_UGT:
            return CmpInst::ICMP_ULE;
        case CmpInst::ICMP_UGE:
            return CmpInst::ICMP_ULT;
        case CmpInst::ICMP_ULT:
            return CmpInst::ICMP_UGE;
        case CmpInst::ICMP_ULE:
            return CmpInst::ICMP_UGT;
        case CmpInst::ICMP_SGT:
            return CmpInst::ICMP_SLE;
        case CmpInst::ICMP_SGE:
            return CmpInst::ICMP_SLT;
        case CmpInst::ICMP_SLT:
            return CmpInst::ICMP_SGE;
        case CmpInst::ICMP_SLE:
            return CmpInst::ICMP_SGT;
        default:
            throw std::runtime_error("Unknown comparison type");
    }
}

std::string CmpInstructionToString(ICmpInst *cmpInst) {
    return "(" +
           getSimpleNodeName(cmpInst->getOperand(0)) + " " +
           cmpPredicateToString(cmpInst->getPredicate()) + " " +
           getSimpleNodeName(cmpInst->getOperand(1))
           + ")";
}

#endif //PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_UTILS_H
