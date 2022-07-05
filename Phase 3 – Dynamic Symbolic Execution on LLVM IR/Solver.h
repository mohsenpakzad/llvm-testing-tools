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
    std::vector<ICmpInst *> comparisonInstructions;
    int minRange, maxRange;
    std::map<std::string, std::set<int>> variablesRange;

    Solver(std::vector<ICmpInst *> comparisonInstructions, int minRange, int maxRange)
            : comparisonInstructions(std::move(comparisonInstructions)), minRange(minRange), maxRange(maxRange) {}

    std::map<std::string, int> solve() {
        applyComparisons();

        // select random number from rage of each key of variablesRange
        std::map<std::string, int> result;
        for (auto &variableRagePair: variablesRange) {
            if (!variableRagePair.second.empty()) {
                auto randomNum = randomInRange(0, variableRagePair.second.size() - 1);
                result[variableRagePair.first] = *std::next(variableRagePair.second.begin(), randomNum);
            } else {
                outs() << "No value for variable " << variableRagePair.first << "\n";
            }
        }

        return result;
    }

    void applyComparisons() {
        for (auto cmpInstruction: comparisonInstructions) {

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

    inline static std::string getLoadInstOperandName(LoadInst *loadInst) {
        return loadInst->getPointerOperand()->getName().str();
    }

};


#endif //PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_SOLVER_H
