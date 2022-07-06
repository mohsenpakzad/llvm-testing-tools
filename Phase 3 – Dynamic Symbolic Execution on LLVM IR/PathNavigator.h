#ifndef PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_PATHNAVIGATOR_H
#define PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_PATHNAVIGATOR_H

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

#include "Utils.h"

using namespace llvm;

class PathNavigator {
private:
    BasicBlock *entryBlock;
    std::map<std::string, int> variablesMap;

    std::vector<BasicBlock *> path;
    std::vector<CmpInstStorePath> cmpInstsStorePath;
public:

    PathNavigator(BasicBlock *entryBlock, std::map<std::string, int> argumentsMap)
            : entryBlock(entryBlock), variablesMap(std::move(argumentsMap)) {}

    void navigate() {
        BasicBlock *currentBasicBlock = entryBlock;
        Instruction * terminatorInst;

        do {
            path.push_back(currentBasicBlock);
            terminatorInst = currentBasicBlock->getTerminator();

            applyAssignments(currentBasicBlock);
            auto evaluateConditionResult = evaluateComparison(currentBasicBlock);
            if (evaluateConditionResult != nullptr) {
                if (*evaluateConditionResult) {
                    currentBasicBlock = terminatorInst->getSuccessor(0);
                } else {
                    currentBasicBlock = terminatorInst->getSuccessor(1);
                }
            } else if (terminatorInst->getNumSuccessors() == 1) {
                currentBasicBlock = terminatorInst->getSuccessor(0);
            } else {
                break;
            }

        } while (true);
    }

    std::map<std::string, int> &getVariablesMap() {
        return variablesMap;
    }

    std::vector<BasicBlock *> &getPath() {
        return path;
    }

    std::vector<CmpInstStorePath> &getCmpInstsStorePath() {
        return cmpInstsStorePath;
    }

private:

    void applyAssignments(BasicBlock *basicBlock) {
        for (auto &I: *basicBlock) {
            if (I.getOpcode() == Instruction::Store) {
                auto *storeInst = dyn_cast<StoreInst>(&I);

                std::string pointerOpName = storeInst->getPointerOperand()->getName().str();
                Value * storeValue = storeInst->getValueOperand();

                // if storeValue is a constant instruction
                // Example: a = 5
                if (isa<ConstantInt>(storeValue)) {
                    variablesMap[pointerOpName] = dyn_cast<ConstantInt>(storeValue)->getSExtValue();
                }
                    // if storeValue is a load instruction
                    // Example: a = b
                else if (isa<LoadInst>(storeValue)) {
                    std::string opName = getLoadInstOperandName(dyn_cast<LoadInst>(storeValue));

                    if (variablesMap.find(opName) == variablesMap.end()) {
                        throw std::runtime_error("Variable " + opName + " not found in variablesMap");
                    }

                    variablesMap[pointerOpName] = variablesMap[opName];
                }
                    // check if store value is binary operator (like add, sub, mul or div)
                    // Example: a = b + c, a = b - 2, a = c * 5, a = 10 / 2
                else if (isa<BinaryOperator>(storeValue)) {
                    variablesMap[pointerOpName] = evaluateBinaryOperation(
                            dyn_cast<BinaryOperator>(storeValue)
                    );
                }
            }
        }
    }

    int evaluateBinaryOperation(BinaryOperator *binaryOperator) {
        Value * op1 = binaryOperator->getOperand(0);
        Value * op2 = binaryOperator->getOperand(1);

        int op1Value = 0;
        int op2Value = 0;

        // check if op1 or op2 is a constant and get constant value
        // or load register and get register value
        if (isa<LoadInst>(op1) && isa<LoadInst>(op2)) {
            std::string op1Name = getLoadInstOperandName(dyn_cast<LoadInst>(op1));
            std::string op2Name = getLoadInstOperandName(dyn_cast<LoadInst>(op2));

            if (variablesMap.find(op1Name) == variablesMap.end() ||
                variablesMap.find(op2Name) == variablesMap.end()) {
                throw std::runtime_error("Variable " + op1Name + " or " + op2Name + "is missing");
            }

            op1Value = variablesMap[op1Name];
            op2Value = variablesMap[op2Name];
        } else if (isa<LoadInst>(op1) && isa<ConstantInt>(op2)) {
            std::string op1Name = getLoadInstOperandName(dyn_cast<LoadInst>(op1));
            int op2ExactValue = dyn_cast<ConstantInt>(op2)->getSExtValue();

            if (variablesMap.find(op1Name) == variablesMap.end()) {
                throw std::runtime_error("Variable " + op1Name + " is missing");
            }

            op1Value = variablesMap[op1Name];
            op2Value = op2ExactValue;
        } else if (isa<ConstantInt>(op1) && isa<LoadInst>(op2)) {
            int op1ExactValue = dyn_cast<ConstantInt>(op1)->getSExtValue();
            std::string op2Name = getLoadInstOperandName(dyn_cast<LoadInst>(op2));

            if (variablesMap.find(op2Name) == variablesMap.end()) {
                throw std::runtime_error("Variable " + op2Name + " is missing");
            }

            op1Value = op1ExactValue;
            op2Value = variablesMap[op2Name];
        } else if (isa<ConstantInt>(op1) && isa<ConstantInt>(op2)) {
            int op1ExactValue = dyn_cast<ConstantInt>(op1)->getSExtValue();
            int op2ExactValue = dyn_cast<ConstantInt>(op2)->getSExtValue();

            op1Value = op1ExactValue;
            op2Value = op2ExactValue;
        }
        return evaluateBinaryOpInstruction(binaryOperator->getOpcode(), op1Value, op2Value);
    }

    static int evaluateBinaryOpInstruction(Instruction::BinaryOps binaryOps, int e1, int e2) {
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
                } else {
                    throw std::runtime_error("Unknown binary operation");
                }
            case Instruction::And:
                return e1 & e2;
            case Instruction::Or:
                return e1 | e2;
            case Instruction::Xor:
                return e1 ^ e2;
            case Instruction::Shl:
                return e1 << e2;
            case Instruction::LShr:
            case Instruction::AShr:
                return e1 >> e2;
            default:
                throw std::runtime_error("Unknown binary operation");
        }
    }

    bool *evaluateComparison(BasicBlock *basicBlock) {
        std::vector<StoreInst *> storeInstsBeforeCmpInst;
        for (auto &I: *basicBlock) {

            if (I.getOpcode() == Instruction::Store){
                storeInstsBeforeCmpInst.push_back(dyn_cast<StoreInst>(&I));
            }
            // Integer comparison
            else if (I.getOpcode() == Instruction::ICmp) {
                auto *cmpInstruction = dyn_cast<ICmpInst>(&I);

                auto opCmp1 = cmpInstruction->getOperand(0);
                auto opCmp2 = cmpInstruction->getOperand(1);

                auto cmpPredicate = cmpInstruction->getPredicate();
                int opCmp1FinalValue = 0;
                int opCmp2FinalValue = 0;

                // Example: a == b, a != b, a < b, a > b, a <= b, a >= b
                if (isa<LoadInst>(opCmp1) && isa<LoadInst>(opCmp2)) {
                    std::string opCmp1Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp1));
                    std::string opCmp2Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp2));

                    if (variablesMap.find(opCmp1Name) == variablesMap.end() ||
                        variablesMap.find(opCmp2Name) == variablesMap.end()) {
                        throw std::runtime_error("Variable " + opCmp1Name + " or " + opCmp2Name + " is missing");
                    }

                    opCmp1FinalValue = variablesMap[opCmp1Name];
                    opCmp2FinalValue = variablesMap[opCmp2Name];
                }
                    // Example: a == 5
                else if (isa<LoadInst>(opCmp1) && isa<ConstantInt>(opCmp2)) {
                    std::string opCmp1Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp1));
                    int opCmp2ExactValue = dyn_cast<ConstantInt>(opCmp2)->getSExtValue();

                    if (variablesMap.find(opCmp1Name) == variablesMap.end()) {
                        throw std::runtime_error("Variable " + opCmp1Name + " is missing");
                    }

                    opCmp1FinalValue = variablesMap[opCmp1Name];
                    opCmp2FinalValue = opCmp2ExactValue;
                }
                    // Example: 7 == b
                else if (isa<ConstantInt>(opCmp1) && isa<LoadInst>(opCmp2)) {
                    int opCmp1ExactValue = dyn_cast<ConstantInt>(opCmp1)->getSExtValue();
                    std::string opCmp2Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp2));

                    if (variablesMap.find(opCmp2Name) == variablesMap.end()) {
                        throw std::runtime_error("Variable " + opCmp2Name + " is missing");
                    }

                    opCmp1FinalValue = opCmp1ExactValue;
                    opCmp2FinalValue = variablesMap[opCmp2Name];
                }
                    // Example: a + b == c + d
                else if (isa<BinaryOperator>(opCmp1) && isa<BinaryOperator>(opCmp2)) {
                    int opCmp1Value = evaluateBinaryOperation(dyn_cast<BinaryOperator>(opCmp1));
                    int opCmp2Value = evaluateBinaryOperation(dyn_cast<BinaryOperator>(opCmp2));

                    opCmp1FinalValue = opCmp1Value;
                    opCmp2FinalValue = opCmp2Value;
                }
                    // Example: a + b == c
                else if (isa<BinaryOperator>(opCmp1) && isa<LoadInst>(opCmp2)) {
                    int opCmp1Value = evaluateBinaryOperation(dyn_cast<BinaryOperator>(opCmp1));
                    std::string opCmp2Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp2));

                    if (variablesMap.find(opCmp2Name) == variablesMap.end()) {
                        throw std::runtime_error("Variable " + opCmp2Name + " is missing");
                    }

                    opCmp1FinalValue = opCmp1Value;
                    opCmp2FinalValue = variablesMap[opCmp2Name];
                }
                    // Example: a == b - c
                else if (isa<LoadInst>(opCmp1) && isa<BinaryOperator>(opCmp2)) {
                    std::string opCmp1Name = getLoadInstOperandName(dyn_cast<LoadInst>(opCmp1));
                    int opCmp2Value = evaluateBinaryOperation(dyn_cast<BinaryOperator>(opCmp2));

                    if (variablesMap.find(opCmp1Name) == variablesMap.end()) {
                        throw std::runtime_error("Variable " + opCmp1Name + " is missing");
                    }

                    opCmp1FinalValue = variablesMap[opCmp1Name];
                    opCmp2FinalValue = opCmp2Value;
                }
                    // Example: a + b == 11
                else if (isa<BinaryOperator>(opCmp1) && isa<ConstantInt>(opCmp2)) {
                    int opCmp1Value = evaluateBinaryOperation(dyn_cast<BinaryOperator>(opCmp1));
                    int opCmp2ExactValue = dyn_cast<ConstantInt>(opCmp2)->getSExtValue();

                    opCmp1FinalValue = opCmp1Value;
                    opCmp2FinalValue = opCmp2ExactValue;
                }
                    // Example: 45 == a * b
                else if (isa<ConstantInt>(opCmp1) && isa<BinaryOperator>(opCmp2)) {
                    int opCmp1ExactValue = dyn_cast<ConstantInt>(opCmp1)->getSExtValue();
                    int opCmp2Value = evaluateBinaryOperation(dyn_cast<BinaryOperator>(opCmp2));

                    opCmp1FinalValue = opCmp1ExactValue;
                    opCmp2FinalValue = opCmp2Value;
                }

                auto cmpResult = evaluateCmpInstruction(cmpPredicate, opCmp1FinalValue, opCmp2FinalValue);

                if (!cmpResult) {
                    cmpInstruction = new ICmpInst(cmpInstruction->getInversePredicate(), opCmp1, opCmp2);
                }
                cmpInstsStorePath.emplace_back(cmpInstruction, storeInstsBeforeCmpInst);
                return new bool(cmpResult);
            }
        }
        return nullptr;
    }

    static bool evaluateCmpInstruction(ICmpInst::Predicate cmpType, int opCmp1Value, int opCmp2Value) {
        switch (cmpType) {
            case ICmpInst::ICMP_EQ:
                return opCmp1Value == opCmp2Value;
            case ICmpInst::ICMP_NE:
                return opCmp1Value != opCmp2Value;
            case ICmpInst::ICMP_UGT:
                return opCmp1Value > opCmp2Value;
            case ICmpInst::ICMP_UGE:
                return opCmp1Value >= opCmp2Value;
            case ICmpInst::ICMP_ULT:
                return opCmp1Value < opCmp2Value;
            case ICmpInst::ICMP_ULE:
                return opCmp1Value <= opCmp2Value;
            case ICmpInst::ICMP_SGT:
                return opCmp1Value > opCmp2Value;
            case ICmpInst::ICMP_SGE:
                return opCmp1Value >= opCmp2Value;
            case ICmpInst::ICMP_SLT:
                return opCmp1Value < opCmp2Value;
            case ICmpInst::ICMP_SLE:
                return opCmp1Value <= opCmp2Value;
            default:
                throw std::runtime_error("Unknown comparison type");
        }
    }
};

#endif //PHASE_3__DYNAMIC_SYMBOLIC_EXECUTION_ON_LLVM_IR_PATHNAVIGATOR_H
