#include <cstdio>
#include <iostream>
#include <set>
#include <iostream>
#include <cstdlib>
#include <ctime>
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

bool VERBOSE_LOG = true;

const int MIN_RANGE = -20;
const int MAX_RANGE = 20;

std::map<std::string, std::set<int>> rangeVariableMap;

LLVMContext &getGlobalContext() {
    static LLVMContext context;
    return context;
}

int randomInRange(int startOfRange, int endOfRange);

std::vector<BasicBlock *> generateRandomPath(BasicBlock *basicBlock);

std::string getSimpleNodeName(const Value *node);

std::tuple<CmpInst::Predicate, std::string, int> *getCmpData(BasicBlock *basicBlock);

void analyzeVariableRange(std::tuple<CmpInst::Predicate, std::string, int> *cmpData);

std::set<int>rangeOperation(const std::set<int> &range1, const std::set<int> &range2,
                            Instruction::BinaryOps binaryOps);

std::set<int> rangeOperation(const std::set<int> &range, int absoluteValue, Instruction::BinaryOps binaryOps);

CmpInst::Predicate getComplementType(CmpInst::Predicate predicate);

int exactValueOperation(int e1, int e2, Instruction::BinaryOps binaryOps);

std::set<int> getOrCreateVariableRange(const std::string& variableName);

std::string getCmpTypeString(CmpInst::Predicate predicate);

void analyzeAssignmentData(BasicBlock *BB);

void printCmpData(std::tuple<CmpInst::Predicate, std::string, int> *cmpData);

int main(int argc, char *argv[]) {
    // Read the IR file.
    LLVMContext & context = getGlobalContext();
    SMDiagnostic err;
    std::unique_ptr<Module> M = parseIRFile(argv[1], err, context);
    if (M == nullptr) {
        fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
        return EXIT_FAILURE;
    }

    for (auto &function: *M) {
        if (function.getName().str() == "main") {
            BasicBlock *mainBasicBlock = dyn_cast<BasicBlock>(function.begin());

            auto path = generateRandomPath(mainBasicBlock);
            llvm::outs() << "************** Path **************" << "\n";
            for (auto &basicBlock: path) {
                llvm::outs() << getSimpleNodeName(basicBlock) << "\n";
            }
        }
    }

    llvm::outs() << "======== Analysis Result =========" << "\n";

    // print data of rangeVariableMap
    for (auto &it: rangeVariableMap) {
        std::cout << it.first << ": ";
        for (auto &i: it.second) {
            std::cout << i << " ";
        }
        std::cout << "\n";
    }

    llvm::outs() << "======= Random Test Output =======" << "\n";

    for (auto &it: rangeVariableMap) {
        unsigned rnd = randomInRange(0, it.second.size() - 1);

        // cast it.second to vector<int>
        std::vector<int> v(it.second.begin(), it.second.end());

        llvm::outs() << it.first << ": " << v[rnd] << "\n";
    }

    return 0;
}

std::tuple<CmpInst::Predicate, std::string, int> *getCmpData(BasicBlock *basicBlock) {

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

            return new std::tuple<CmpInst::Predicate, std::string, int>(cmpType, cmpVariableName, cmpImmediateValue);
        }
    }
    return nullptr;
}

void analyzeVariableRange(std::tuple<CmpInst::Predicate, std::string, int> *cmpData) {
    CmpInst::Predicate cmpType = std::get<0>(*cmpData);
    std::string cmpVariableName = std::get<1>(*cmpData);
    int cmpImmediateValue = std::get<2>(*cmpData);

    if (cmpVariableName.empty()) {
        return;
    }

    if (rangeVariableMap.find(cmpVariableName) == rangeVariableMap.end()) {
        rangeVariableMap[cmpVariableName] = std::set<int>();
        for (int i = MIN_RANGE; i <= MAX_RANGE; i++) {
            rangeVariableMap[cmpVariableName].insert(i);
        }
    }

    switch (cmpType) {
        case CmpInst::ICMP_EQ:
            rangeVariableMap[cmpVariableName].insert(cmpImmediateValue);
            break;
        case CmpInst::ICMP_NE:
            for (auto it = rangeVariableMap[cmpVariableName].begin(); it != rangeVariableMap[cmpVariableName].end();) {
                if (*it == cmpImmediateValue) {
                    it = rangeVariableMap[cmpVariableName].erase(it);
                } else {
                    ++it;
                }
            }
            break;
        case CmpInst::ICMP_UGT:
        case CmpInst::ICMP_SGT:
            for (auto it = rangeVariableMap[cmpVariableName].begin(); it != rangeVariableMap[cmpVariableName].end();) {
                if (*it <= cmpImmediateValue) {
                    it = rangeVariableMap[cmpVariableName].erase(it);
                } else {
                    ++it;
                }
            }
            break;
        case CmpInst::ICMP_UGE:
        case CmpInst::ICMP_SGE:
            for (auto it = rangeVariableMap[cmpVariableName].begin(); it != rangeVariableMap[cmpVariableName].end();) {
                if (*it < cmpImmediateValue) {
                    it = rangeVariableMap[cmpVariableName].erase(it);
                } else {
                    ++it;
                }
            }
            break;
        case CmpInst::ICMP_ULT:
        case CmpInst::ICMP_SLT:
            for (auto it = rangeVariableMap[cmpVariableName].begin(); it != rangeVariableMap[cmpVariableName].end();) {
                if (*it >= cmpImmediateValue) {
                    it = rangeVariableMap[cmpVariableName].erase(it);
                } else {
                    ++it;
                }
            }
            break;
        case CmpInst::ICMP_ULE:
        case CmpInst::ICMP_SLE:
            for (auto it = rangeVariableMap[cmpVariableName].begin(); it != rangeVariableMap[cmpVariableName].end();) {
                if (*it > cmpImmediateValue) {
                    it = rangeVariableMap[cmpVariableName].erase(it);
                } else {
                    ++it;
                }
            }
            break;
        default:
            break;
    }
}

std::set<int> rangeOperation(const std::set<int> &range1, const std::set<int> &range2,
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

std::set<int> rangeOperation(const std::set<int> &range, int absoluteValue, Instruction::BinaryOps binaryOps) {
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

std::vector<BasicBlock *> generateRandomPath(BasicBlock *basicBlock) {
    std::vector<BasicBlock *> path = {basicBlock};
    Instruction * terminatorInst = basicBlock->getTerminator();
    unsigned numberOfSuccessors = terminatorInst->getNumSuccessors();

    if (VERBOSE_LOG) {
        llvm::outs() << "----------- Conditions -----------" << "\n";
    }

    while (numberOfSuccessors > 0) {
        BasicBlock *successor;
        if (numberOfSuccessors == 1) {
            successor = terminatorInst->getSuccessor(0);
//            llvm::outs() << getSimpleNodeName(successor) << "\n";
        } else {
            unsigned rnd = randomInRange(0, numberOfSuccessors - 1);
            successor = terminatorInst->getSuccessor(rnd);
//            llvm::outs() << getSimpleNodeName(successor) << "\n";

            auto *cmpData = getCmpData(path.back());
            if (cmpData != nullptr) {

                // get all elements of cmpData
                CmpInst::Predicate cmpType;
                std::string cmpVariableName;
                int cmpImmediateValue;
                std::tie(cmpType, cmpVariableName, cmpImmediateValue) = *cmpData;

                // if we are in false branch complement the cmp type
                if (rnd == 1) {
                    auto complementCmpType = getComplementType(cmpType);
                    std::get<0>(*cmpData) = complementCmpType;
                }
                analyzeVariableRange(cmpData);

                if (VERBOSE_LOG) {
                    llvm::outs() << getSimpleNodeName(successor) << "\n";
                    printCmpData(cmpData);
                    llvm::outs() << "----------------------------------" << "\n";
                }

                analyzeAssignmentData(successor);
                delete cmpData;
            }
        }
        path.push_back(successor);
        terminatorInst = successor->getTerminator();
        numberOfSuccessors = terminatorInst->getNumSuccessors();
    }
    return path;
}

std::string getSimpleNodeName(const Value *node) {
    if (!node->getName().empty())
        return node->getName().str();
    std::string str;
    raw_string_ostream os(str);
    node->printAsOperand(os, false);
    return os.str();
}

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

CmpInst::Predicate getComplementType(CmpInst::Predicate predicate) {
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

void analyzeAssignmentData(BasicBlock *BB) {
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
        if (isa<StoreInst>(I)) {
            auto *storeInst = dyn_cast<StoreInst>(I);

            Value * pointerOperand = storeInst->getPointerOperand();
            std::string pointerOperandName = pointerOperand->getName().str();
//            llvm::outs() << "pointerOperandName: " << pointerOperandName << "\n";

            Value * storeValue = storeInst->getValueOperand();
//            storeValue->print(llvm::outs());
//            llvm::outs() << "\n";


            // if storeValue is a constant instruction, just assign it new range
            if (isa<ConstantInt>(storeValue)) {
                auto *constantInt = dyn_cast<ConstantInt>(storeValue);

                std::set<int> newRange;
                newRange.insert(constantInt->getSExtValue());
                rangeVariableMap[pointerOperandName] = newRange;

                // check if store value is add, sub, mul or div and in the base of that
                // then calculate the range of pointerOperand base on operands of store value
                // if storeValue is a binary operator
            } else if (isa<BinaryOperator>(storeValue)) {
                auto *binaryOperator = dyn_cast<BinaryOperator>(storeValue);
                Value * op1 = binaryOperator->getOperand(0);
                Value * op2 = binaryOperator->getOperand(1);

                // print the operation type of  the storeValue
//                llvm::outs() << binaryOperator->getOpcodeName() << "\n";



                // check if op1 or op2 is a constant and get constant value
                // or load register and get register value
                if (isa<LoadInst>(op1) && isa<LoadInst>(op2)) {
                    auto *loadOp1 = dyn_cast<LoadInst>(op1);
                    auto *loadOp2 = dyn_cast<LoadInst>(op2);
                    std::string op1Name = loadOp1->getPointerOperand()->getName().str();
                    std::string op2Name = loadOp2->getPointerOperand()->getName().str();

                    std::set<int> op1Range = getOrCreateVariableRange(op1Name);
                    std::set<int> op2Range = getOrCreateVariableRange(op2Name);
                    rangeVariableMap[pointerOperandName] = rangeOperation(
                            op1Range, op2Range, binaryOperator->getOpcode()
                    );
                } else if (isa<LoadInst>(op1) && isa<ConstantInt>(op2)) {
                    auto *loadOp1 = dyn_cast<LoadInst>(op1);
                    auto *constantOp2 = dyn_cast<ConstantInt>(op2);
                    std::string op1Name = loadOp1->getPointerOperand()->getName().str();
                    std::set<int> op1Range = getOrCreateVariableRange(op1Name);

                    rangeVariableMap[pointerOperandName] = rangeOperation(
                            op1Range, constantOp2->getSExtValue(), binaryOperator->getOpcode()
                    );
                } else if (isa<ConstantInt>(op1) && isa<LoadInst>(op2)) {
                    auto *constantOp1 = dyn_cast<ConstantInt>(op1);
                    auto *loadOp2 = dyn_cast<LoadInst>(op2);
                    std::string op2Name = loadOp2->getPointerOperand()->getName().str();
                    std::set<int> op2Range = getOrCreateVariableRange(op2Name);

                    rangeVariableMap[pointerOperandName] = rangeOperation(
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
                    rangeVariableMap[pointerOperandName] = newRange;
                }
            }
        }
    }
}

std::set<int> getOrCreateVariableRange(const std::string &variableName) {
    if (rangeVariableMap.find(variableName) == rangeVariableMap.end()) {
        rangeVariableMap[variableName] = std::set<int>();
        for (int i = MIN_RANGE; i <= MAX_RANGE; i++) {
            rangeVariableMap[variableName].insert(i);
        }
        return rangeVariableMap[variableName];
    }
    return rangeVariableMap[variableName];
}

int exactValueOperation(int e1, int e2, Instruction::BinaryOps binaryOps) {
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

void printCmpData(std::tuple<CmpInst::Predicate, std::string, int> *cmpData) {
    CmpInst::Predicate predicate = std::get<0>(*cmpData);
    std::string variableName = std::get<1>(*cmpData);
    int immediateValue = std::get<2>(*cmpData);
    llvm::outs() << variableName << " " << getCmpTypeString(predicate) << " " << immediateValue << "\n";
}

std::string getCmpTypeString(CmpInst::Predicate predicate) {
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
