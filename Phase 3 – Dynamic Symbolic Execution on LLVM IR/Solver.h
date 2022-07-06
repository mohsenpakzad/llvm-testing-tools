#ifndef PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_SOLVER_H
#define PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_SOLVER_H

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
#include "Utils.h"

using namespace llvm;

class Solver {
public:
    std::vector<CmpInstStorePath> cmpInstsStorePath;
    int minRange, maxRange;
    std::map<std::string, std::set<int>> variablesRange;

    Solver(std::vector<CmpInstStorePath> cmpInstsStorePath, int minRange, int maxRange)
            : cmpInstsStorePath(std::move(cmpInstsStorePath)), minRange(minRange), maxRange(maxRange) {}

    std::map<std::string, int> solve() {
        applyComparisons();

        // select random number from rage of each key of variablesRange
        std::map<std::string, int> result;
        for (auto &variableRagePair: variablesRange) {
            if (!variableRagePair.second.empty()) {
                auto randomNum = randomInRange(0, variableRagePair.second.size() - 1);
                result[variableRagePair.first] = *std::next(variableRagePair.second.begin(), randomNum);
            } else {
                // print variable name and range
                outs() << "!!!!!!!No value for variable " << variableRagePair.first << "!!!!!!!\n";
            }
        }

        return result;
    }

    void applyComparisons() {
        for (const auto &cmpInstStorePath: cmpInstsStorePath) {

            // print variablesRange
            outs() << "before variablesRange: \n";
            for (auto &variableRangePair: variablesRange) {
                outs() << variableRangePair.first << ": ";
                for (auto &range: variableRangePair.second) {
                    outs() << range << " ";
                }
                outs() << "\n";
            }

            applyStorePathAssignments(cmpInstStorePath.storePath);

            outs() << "after variablesRange: \n";
            for (auto &variableRangePair: variablesRange) {
                outs() << variableRangePair.first << ": ";
                for (auto &range: variableRangePair.second) {
                    outs() << range << " ";
                }
                outs() << "\n";
            }

            auto cmpInstruction = cmpInstStorePath.cmpInst;

            auto opCmp1 = cmpInstruction->getOperand(0);
            auto opCmp2 = cmpInstruction->getOperand(1);

            auto cmpPredicate = cmpInstruction->getPredicate();
            std::pair<std::string, std::set<int>> opCmp1FinalRange;
            std::pair<std::string, std::set<int>> opCmp2FinalRange;

            // Example: a == b, a != b, a < b, a > b, a <= b, a >= b
            if (isa<LoadInst>(opCmp1) && isa<LoadInst>(opCmp2)) {
                std::string opCmp1Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp1));
                std::string opCmp2Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp2));

                opCmp1FinalRange = std::make_pair(opCmp1Name, getVariableRange(opCmp1Name));
                opCmp2FinalRange = std::make_pair(opCmp2Name, getVariableRange(opCmp2Name));
            }
                // Example: a == 5
            else if (isa<LoadInst>(opCmp1) && isa<ConstantInt>(opCmp2)) {
                std::string opCmp1Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp1));
                int opCmp2ExactValue = dyn_cast<ConstantInt>(opCmp2)->getSExtValue();

                opCmp1FinalRange = std::make_pair(opCmp1Name, getVariableRange(opCmp1Name));
                opCmp2FinalRange = std::make_pair("", std::set<int>{opCmp2ExactValue});
            }
                // Example: 7 == b
            else if (isa<ConstantInt>(opCmp1) && isa<LoadInst>(opCmp2)) {
                int opCmp1ExactValue = dyn_cast<ConstantInt>(opCmp1)->getSExtValue();
                std::string opCmp2Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp2));

                opCmp1FinalRange = std::make_pair("", std::set<int>{opCmp1ExactValue});
                opCmp2FinalRange = std::make_pair(opCmp2Name, getVariableRange(opCmp2Name));
            }


            applyCmpInstToVariablesRange(cmpPredicate, opCmp1FinalRange, opCmp2FinalRange);
        }

    }

    void applyStorePathAssignments(std::vector<StoreInst *> storePath) {
        for (auto storeInst: storePath) {
            std::string pointerOpName = storeInst->getPointerOperand()->getName().str();
            Value * storeValue = storeInst->getValueOperand();

//            if (variablesRange.find(pointerOpName) == variablesRange.end()) continue;

            // if storeValue is a constant instruction
            // Example: a = 5
            if (isa<ConstantInt>(storeValue)) {
                auto exactValue = dyn_cast<ConstantInt>(storeValue)->getSExtValue();

                variablesRange[pointerOpName] = std::set<int>();
                variablesRange[pointerOpName].insert(exactValue);
            }
                // if storeValue is a load instruction
                // Example: a = b
            else if (isa<LoadInst>(storeValue)) {
                std::string opName = getLoadInstOperandName(dyn_cast<LoadInst>(storeValue));

//                if (variablesRange.find(opName) != variablesRange.end()) {
                    variablesRange[pointerOpName] = getVariableRange(opName);
//                }
            }
                // check if store value is binary operator (like add, sub, mul or div)
                // Example: a = b + c, a = b - 2, a = c * 5, a = 10 / 2
            else if (isa<BinaryOperator>(storeValue)) {
                outs() << "Reached!\n";
                applyBinaryOperationAssignment(pointerOpName, dyn_cast<BinaryOperator>(storeValue));
            }
        }
    }

    void applyBinaryOperationAssignment(const std::string& pointerOpName, BinaryOperator *binaryOperator) {
        Value * op1 = binaryOperator->getOperand(0);
        Value * op2 = binaryOperator->getOperand(1);

        // check if op1 or op2 is a constant and get constant value
        // or load register and get register value
        if (isa<LoadInst>(op1) && isa<LoadInst>(op2)) {
            std::string op1Name = getLoadInstOperandName(dyn_cast<LoadInst>(op1));
            std::string op2Name = getLoadInstOperandName(dyn_cast<LoadInst>(op2));

//            if (variablesRange.find(op1Name) != variablesRange.end() &&
//                    variablesRange.find(op2Name) != variablesRange.end()) {
                variablesRange[pointerOpName] = rangeOperation(
                        binaryOperator->getOpcode(),
                        getVariableRange(op1Name),
                        getVariableRange(op2Name)
               );
//            }
        } else if (isa<LoadInst>(op1) && isa<ConstantInt>(op2)) {
            std::string op1Name = getLoadInstOperandName(dyn_cast<LoadInst>(op1));
            int op2ExactValue = dyn_cast<ConstantInt>(op2)->getSExtValue();

//            if (variablesRange.find(op1Name) != variablesRange.end()) {
                variablesRange[pointerOpName] = rangeOperation(
                        binaryOperator->getOpcode(),
                        getVariableRange(op1Name),
                        std::set<int>{op2ExactValue}
                );
//            }
        } else if (isa<ConstantInt>(op1) && isa<LoadInst>(op2)) {
            int op1ExactValue = dyn_cast<ConstantInt>(op1)->getSExtValue();
            std::string op2Name = getLoadInstOperandName(dyn_cast<LoadInst>(op2));

//            if (variablesRange.find(op2Name) != variablesRange.end()) {
                variablesRange[pointerOpName] = rangeOperation(
                        binaryOperator->getOpcode(),
                        std::set<int>{op1ExactValue},
                        getVariableRange(op2Name)
                );
//            }
        } else if (isa<ConstantInt>(op1) && isa<ConstantInt>(op2)) {
            int op1ExactValue = dyn_cast<ConstantInt>(op1)->getSExtValue();
            int op2ExactValue = dyn_cast<ConstantInt>(op2)->getSExtValue();

            variablesRange[pointerOpName] = rangeOperation(
                    binaryOperator->getOpcode(),
                    std::set<int>{op1ExactValue},
                    std::set<int>{op2ExactValue}
            );
        }
    }


    void applyCmpInstToVariablesRange(CmpInst::Predicate cmpPredicate,
                                      const std::pair<std::string, std::set<int>> &opCmp1Range,
                                      const std::pair<std::string, std::set<int>> &opCmp2Range) {

        // 1. Find the range that both ranges makes cmpPredicate true
        std::set<int> cmpResultRange;

        for (auto opCmp1RangeElement: opCmp1Range.second) {
            for (auto opCmp2RangeElement: opCmp2Range.second) {
                switch (cmpPredicate) {
                    case CmpInst::ICMP_EQ:
                        if (opCmp1RangeElement == opCmp2RangeElement) {
                            cmpResultRange.insert(opCmp1RangeElement);
                        }
                        break;
                    case CmpInst::ICMP_NE:
                        if (opCmp1RangeElement != opCmp2RangeElement) {
                            cmpResultRange.insert(opCmp1RangeElement);
                        }
                        break;
                    case CmpInst::ICMP_UGT:
                    case CmpInst::ICMP_SGT:
                        if (opCmp1RangeElement > opCmp2RangeElement) {
                            cmpResultRange.insert(opCmp1RangeElement);
                        }
                        break;
                    case CmpInst::ICMP_UGE:
                    case CmpInst::ICMP_SGE:
                        if (opCmp1RangeElement >= opCmp2RangeElement) {
                            cmpResultRange.insert(opCmp1RangeElement);
                        }
                        break;
                    case CmpInst::ICMP_ULT:
                    case CmpInst::ICMP_SLT:
                        if (opCmp1RangeElement < opCmp2RangeElement) {
                            cmpResultRange.insert(opCmp1RangeElement);
                        }
                        break;
                    case CmpInst::ICMP_ULE:
                    case CmpInst::ICMP_SLE:
                        if (opCmp1RangeElement <= opCmp2RangeElement) {
                            cmpResultRange.insert(opCmp1RangeElement);
                        }
                        break;
                    default:
                        throw std::runtime_error("Unknown CmpInst::Predicate");
                }

            }
        }


        // 2. Update the range of the variable
        if (!opCmp1Range.first.empty()) {
            variablesRange[opCmp1Range.first] = cmpResultRange;
        }

        if (!opCmp2Range.first.empty()) {
            variablesRange[opCmp2Range.first] = cmpResultRange;
        }
    }

    std::set<int> getVariableRange(const std::string &variableName) {
        if (variablesRange.find(variableName) == variablesRange.end()) {
            variablesRange[variableName] = std::set<int>();
            for (int i = minRange; i <= maxRange; i++) {
                variablesRange[variableName].insert(i);
            }
            return variablesRange[variableName];
        }
        return variablesRange[variableName];
    }


    static std::set<int> rangeOperation(Instruction::BinaryOps binaryOps,
                                        const std::set<int> &range1, const std::set<int> &range2) {
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
                throw std::runtime_error("Unknown Instruction::BinaryOps");
        }
        return result;
    }


    inline static std::string getLoadInstOperandName(LoadInst *loadInst) {
        return loadInst->getPointerOperand()->getName().str();
    }

};


#endif //PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_SOLVER_H
