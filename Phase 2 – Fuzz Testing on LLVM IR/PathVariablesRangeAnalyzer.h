#ifndef PHASE_1__RANDOM_TESTING_ON_LLVM_IR_PATHVARIABLESRANGEANALYZER_H
#define PHASE_1__RANDOM_TESTING_ON_LLVM_IR_PATHVARIABLESRANGEANALYZER_H

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

using namespace llvm;

class PathVariablesRangeAnalyzer {
private:
    const std::vector<BasicBlock *> &path;
    const int minRange, maxRange;
    std::map<std::string, std::set<int>> variablesRangeMap;

    /**
     * @brief Finds the range of the variable in the path
     * @param cmpData
     */
    void analyzeVariableRange(std::tuple<CmpInst::Predicate, std::string, int> *cmpData) {
        CmpInst::Predicate cmpType = std::get<0>(*cmpData);
        std::string cmpVariableName = std::get<1>(*cmpData);
        int cmpImmediateValue = std::get<2>(*cmpData);

        // if no variable presented or the variable name not in the map of variables exits the function
        if (cmpVariableName.empty() || variablesRangeMap.find(cmpVariableName) == variablesRangeMap.end()) {
            return;
        }

        // then remove the range of the variable according to the comparison
        for (auto it = variablesRangeMap[cmpVariableName].begin();
             it != variablesRangeMap[cmpVariableName].end();) {
            switch (cmpType) {
                case CmpInst::ICMP_EQ:
                    if (*it != cmpImmediateValue) {
                        it = variablesRangeMap[cmpVariableName].erase(it);
                    } else {
                        ++it;
                    }
                    break;
                case CmpInst::ICMP_NE:
                    if (*it == cmpImmediateValue) {
                        it = variablesRangeMap[cmpVariableName].erase(it);
                    } else {
                        ++it;
                    }
                    break;
                case CmpInst::ICMP_UGT:
                case CmpInst::ICMP_SGT:
                    if (*it <= cmpImmediateValue) {
                        it = variablesRangeMap[cmpVariableName].erase(it);
                    } else {
                        ++it;
                    }
                    break;
                case CmpInst::ICMP_UGE:
                case CmpInst::ICMP_SGE:
                    if (*it < cmpImmediateValue) {
                        it = variablesRangeMap[cmpVariableName].erase(it);
                    } else {
                        ++it;
                    }
                    break;
                case CmpInst::ICMP_ULT:
                case CmpInst::ICMP_SLT:
                    if (*it >= cmpImmediateValue) {
                        it = variablesRangeMap[cmpVariableName].erase(it);
                    } else {
                        ++it;
                    }
                    break;
                case CmpInst::ICMP_ULE:
                case CmpInst::ICMP_SLE:
                    if (*it > cmpImmediateValue) {
                        it = variablesRangeMap[cmpVariableName].erase(it);
                    } else {
                        ++it;
                    }
                    break;
                default:
                    outs() << "Unknown comparison type: " << cmpType << '\n';
                    exit(1);
            }
        }
    }

    /**
     * @brief Get basic block and apply assignments to the variables range map
     * @param basicBlock
     * @return
     */
    void analyzeAssignmentData(BasicBlock *BB) {
        for (auto I = BB->begin(), E = BB->end(); I != E; ++I) {
            if (isa<StoreInst>(I)) {
                auto *storeInst = dyn_cast<StoreInst>(I);

                Value * pointerOperand = storeInst->getPointerOperand();
                std::string pointerOperandName = pointerOperand->getName().str();
//            outs() << "pointerOperandName: " << pointerOperandName << "\n";

                Value * storeValue = storeInst->getValueOperand();
//            storeValue->print(outs());
//            outs() << "\n";


                // if storeValue is a constant instruction, just assign it new range
                if (isa<ConstantInt>(storeValue)) {
                    auto *constantInt = dyn_cast<ConstantInt>(storeValue);

                    std::set<int> newRange;
                    newRange.insert(constantInt->getSExtValue());
                    variablesRangeMap[pointerOperandName] = newRange;

                    // check if store value is add, sub, mul or div and in the base of that
                    // then calculate the range of pointerOperand base on operands of store value
                    // if storeValue is a binary operator
                } else if (isa<BinaryOperator>(storeValue)) {
                    auto *binaryOperator = dyn_cast<BinaryOperator>(storeValue);
                    Value * op1 = binaryOperator->getOperand(0);
                    Value * op2 = binaryOperator->getOperand(1);

                    // print the operation type of  the storeValue
//                outs() << binaryOperator->getOpcodeName() << "\n";



                    // check if op1 or op2 is a constant and get constant value
                    // or load register and get register value
                    if (isa<LoadInst>(op1) && isa<LoadInst>(op2)) {
                        auto *loadOp1 = dyn_cast<LoadInst>(op1);
                        auto *loadOp2 = dyn_cast<LoadInst>(op2);
                        std::string op1Name = loadOp1->getPointerOperand()->getName().str();
                        std::string op2Name = loadOp2->getPointerOperand()->getName().str();

                        // if op1Name or op2Name don't exist in variableRangeMap, exit the function
                        if (variablesRangeMap.find(op1Name) == variablesRangeMap.end() ||
                            variablesRangeMap.find(op2Name) == variablesRangeMap.end()) {
                            return;
                        }

                        std::set<int> op1Range = variablesRangeMap[op1Name];
                        std::set<int> op2Range = variablesRangeMap[op2Name];
                        variablesRangeMap[pointerOperandName] = rangeOperation(
                                op1Range, op2Range, binaryOperator->getOpcode()
                        );
                    } else if (isa<LoadInst>(op1) && isa<ConstantInt>(op2)) {
                        auto *loadOp1 = dyn_cast<LoadInst>(op1);
                        auto *constantOp2 = dyn_cast<ConstantInt>(op2);
                        std::string op1Name = loadOp1->getPointerOperand()->getName().str();

                        // if op1Name don't exist in variableRangeMap, exit the function
                        if (variablesRangeMap.find(op1Name) == variablesRangeMap.end()) {
                            return;
                        }
                        std::set<int> op1Range = variablesRangeMap[op1Name];

                        variablesRangeMap[pointerOperandName] = rangeOperation(
                                op1Range, constantOp2->getSExtValue(), binaryOperator->getOpcode()
                        );
                    } else if (isa<ConstantInt>(op1) && isa<LoadInst>(op2)) {
                        auto *constantOp1 = dyn_cast<ConstantInt>(op1);
                        auto *loadOp2 = dyn_cast<LoadInst>(op2);
                        std::string op2Name = loadOp2->getPointerOperand()->getName().str();

                        // if op2Name don't exist in variableRangeMap, exit the function
                        if (variablesRangeMap.find(op2Name) == variablesRangeMap.end()) {
                            return;
                        }
                        std::set<int> op2Range = variablesRangeMap[op2Name];

                        variablesRangeMap[pointerOperandName] = rangeOperation(
                                op2Range, constantOp1->getSExtValue(), binaryOperator->getOpcode()
                        );
                    } else if (isa<ConstantInt>(op1) && isa<ConstantInt>(op2)) {
                        auto *constantOp1 = dyn_cast<ConstantInt>(op1);
                        auto *constantOp2 = dyn_cast<ConstantInt>(op2);

                        std::set<int> newRange;
                        newRange.insert(exactValueOperation(
                                constantOp1->getSExtValue(),
                                constantOp2->getSExtValue(),
                                binaryOperator->getOpcode()
                        ));
                        variablesRangeMap[pointerOperandName] = newRange;
                    }
                }
            }
        }
    }

    /**
     * @brief get basic block and return comparison data of block if its present
     * @param basicBlock
     * @return comparison data of block if its present
     */
    static std::tuple<CmpInst::Predicate, std::string, int> *getCmpData(BasicBlock *basicBlock) {

        if (basicBlock == nullptr) return nullptr;

        // print all cmp instructions
        for (auto &I: *basicBlock) {
            if (isa<ICmpInst>(I)) {
                auto *cmp = dyn_cast<ICmpInst>(&I);

                auto opCmp1 = cmp->getOperand(0);
                auto opCmp2 = cmp->getOperand(1);

                // get compare type of ICmpInst in string format and print it
                auto cmpType = cmp->getPredicate();
                std::string cmpVariableName;
                int cmpImmediateValue;

                if (isa<LoadInst>(opCmp1)) {
                    auto *loadInst = dyn_cast<LoadInst>(opCmp1);
                    cmpVariableName = loadInst->getPointerOperand()->getName().str();
                } else {
                    cmpImmediateValue = std::stoi(getSimpleNodeName(opCmp1));
                }

                if (isa<LoadInst>(opCmp2)) {
                    auto *loadInst = dyn_cast<LoadInst>(opCmp2);
                    cmpVariableName = loadInst->getPointerOperand()->getName().str();
                } else {
                    cmpImmediateValue = std::stoi(getSimpleNodeName(opCmp2));
                }

                return new std::tuple<CmpInst::Predicate, std::string, int>(cmpType, cmpVariableName,
                                                                            cmpImmediateValue);
            }
        }
        return nullptr;
    }

    /**
     * @brief get comparison predicate and complement it
     * @param predicate
     * @return complement predicate of comparison
     */
    static CmpInst::Predicate getComplementType(CmpInst::Predicate predicate) {
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
                return CmpInst::Predicate::ICMP_EQ;
        }
    }

    // ============================== operations ==============================

    static int exactValueOperation(int e1, int e2, Instruction::BinaryOps binaryOps) {
        switch (binaryOps) {
            case Instruction::Add:
                return e1 + e2;
            case Instruction::Sub:
                return e1 - e2;
            case Instruction::Mul:
                return e1 * e2;
            case Instruction::SDiv:
                if (e2 != 0) {
                    return e1 / e2;
                }
            default:
                return 0; // todo: unsupported operation
        }
    }

    static std::set<int> rangeOperation(const std::set<int> &range1, const std::set<int> &range2,
                                        Instruction::BinaryOps binaryOps) {
        std::set<int> result;
        switch (binaryOps) {
            case Instruction::Add:
                for (int e1: range1) {
                    for (int e2: range2) {
                        result.insert(e1 + e2);
                    }
                }
                break;
            case Instruction::Sub:
                for (int e1: range1) {
                    for (int e2: range2) {
                        result.insert(e1 - e2);
                    }
                }
                break;
            case Instruction::Mul:
                for (int e1: range1) {
                    for (int e2: range2) {
                        result.insert(e1 * e2);
                    }
                }
                break;
            case Instruction::SDiv:
                for (int e1: range1) {
                    for (int e2: range2) {
                        if (e2 != 0) {
                            result.insert(e1 / e2);
                        }
                    }
                }
                break;
            default:
                return result; // todo: unsupported operation
        }
        return result;
    }

    static std::set<int> rangeOperation(const std::set<int> &range, int absoluteValue,
                                        Instruction::BinaryOps binaryOps) {
        std::set<int> result;
        switch (binaryOps) {
            case Instruction::Add:
                for (int e: range) {
                    result.insert(e + absoluteValue);
                }
                break;
            case Instruction::Sub:
                for (int e: range) {
                    result.insert(e - absoluteValue);
                }
                break;
            case Instruction::Mul:
                for (int e: range) {
                    result.insert(e * absoluteValue);
                }
                break;
            case Instruction::SDiv:
                for (int e: range) {
                    if (absoluteValue != 0) {
                        result.insert(e / absoluteValue);
                    }
                }
                break;
            default:
                return result; // todo: unsupported operation
        }
        return result;
    }

    // ==========================================================================

    // ====================== print comparison data stuff  ======================

    /**
     * @brief get comparison type as string
     * @param predicate
     * @return string representation of comparison type
     */
    static std::string getCmpTypeString(CmpInst::Predicate predicate) {
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

    /**
     * @brief get comparison type as string
     * @param predicate
     * @return string representation of comparison type
     */
    static void printCmpData(std::tuple<CmpInst::Predicate, std::string, int> *cmpData) {
        CmpInst::Predicate predicate = std::get<0>(*cmpData);
        std::string variableName = std::get<1>(*cmpData);
        int immediateValue = std::get<2>(*cmpData);
        outs() << variableName << " " << getCmpTypeString(predicate) << " " << immediateValue << "\n";
    }

    // ==========================================================================

public:
    explicit PathVariablesRangeAnalyzer(std::vector<BasicBlock *> &path, int minRange, int maxRange)
            : path(path), minRange(minRange), maxRange(maxRange) {

        outs() << "----------- Conditions -----------" << "\n";

        std::map<BasicBlock *, std::tuple<CmpInst::Predicate, std::string, int> *> cmpDataOfBlocks;

        for (BasicBlock *bb: path) {
            BasicBlock *prevBB = bb->getSinglePredecessor();
            auto cmpData = getCmpData(prevBB);
            cmpDataOfBlocks[prevBB] = cmpData;

            if (cmpData == nullptr) continue;

            std::string cmpVariableName = std::get<1>(*cmpData);
            // if the variable is not in the map, add it with its range
            if (variablesRangeMap.find(cmpVariableName) == variablesRangeMap.end()) {
                variablesRangeMap[cmpVariableName] = std::set<int>();
                for (int i = minRange; i <= maxRange; i++) {
                    variablesRangeMap[cmpVariableName].insert(i);
                }
            }
        }

        for (BasicBlock *bb: path) {

            // get bb previous basic block
            BasicBlock *prevBB = bb->getSinglePredecessor();
            auto cmpData = cmpDataOfBlocks[prevBB];
            if (prevBB && cmpData != nullptr) {
                // get all elements of cmpData
                CmpInst::Predicate cmpType;
                std::string cmpVariableName;
                int cmpImmediateValue;
                std::tie(cmpType, cmpVariableName, cmpImmediateValue) = *cmpData;

                // if we are in false branch complement the cmp type
                if (prevBB->getTerminator()->getNumSuccessors() == 2 &&
                    prevBB->getTerminator()->getSuccessor(1) == bb) {
                    auto complementCmpType = getComplementType(cmpType);
                    std::get<0>(*cmpData) = complementCmpType;
                }

                analyzeVariableRange(cmpData);

                outs() << getSimpleNodeName(bb) << "\n";
                printCmpData(cmpData);
                outs() << "----------------------------------" << "\n";


                // TODO: fix this later!
//                analyzeAssignmentData(bb);
            }
        }
    }

    std::map<std::string, std::set<int>> getVariablesRangeMap() const {
        return variablesRangeMap;
    }
};

#endif //PHASE_1__RANDOM_TESTING_ON_LLVM_IR_PATHVARIABLESRANGEANALYZER_H
