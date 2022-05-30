#include <cstdio>
#include <iostream>
#include <set>
#include <iostream>
#include <cstdlib>
#include <ctime>
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

using namespace llvm;

bool VERBOSE_LOG = true;

const int MIN_RANGE = -20;
const int MAX_RANGE = 20;
const int MAX_GENERATIONS = 5000;

BasicBlock *mainBasicBlock;
std::set<BasicBlock *> allBlocks;
std::map<std::string, std::set<int>> rangeVariableMap;


LLVMContext &getGlobalContext() {
    static LLVMContext context;
    return context;
}

int randomInRange(int startOfRange, int endOfRange);

void printRange(const std::vector<BasicBlock *> &path);

std::vector<BasicBlock *> generateRandomPath(BasicBlock *basicBlock);

std::string getSimpleNodeName(const Value *node);

std::shared_ptr<std::tuple<CmpInst::Predicate, std::string, int>> getCmpData(BasicBlock *basicBlock);

void analyzeVariableRange(std::shared_ptr<std::tuple<CmpInst::Predicate, std::string, int>> cmpData);

std::set<int> rangeOperation(const std::set<int> &range1, const std::set<int> &range2,
                             Instruction::BinaryOps binaryOps);

std::set<int> rangeOperation(const std::set<int> &range, int absoluteValue, Instruction::BinaryOps binaryOps);

CmpInst::Predicate getComplementType(CmpInst::Predicate predicate);

int exactValueOperation(int e1, int e2, Instruction::BinaryOps binaryOps);

std::set<int> getOrCreateVariableRange(const std::string &variableName);

std::string getCmpTypeString(CmpInst::Predicate predicate);

void analyzeAssignmentData(BasicBlock *BB);

void printCmpData(std::shared_ptr<std::tuple<CmpInst::Predicate, std::string, int>> cmpData);


class Chromosome {
private:
    std::vector<std::vector<BasicBlock *>> pathList;

    static std::vector<std::vector<BasicBlock *>> selectRandomNumberOfPaths(const Chromosome *chromosome) {
        std::vector<std::vector<BasicBlock *>> newPathList;
        for (const auto &path: chromosome->pathList) {
            if (randomInRange(0, 1)) {
                newPathList.push_back(path);
            }
        }
        return newPathList;
    }

public:
     Chromosome(std::vector<std::vector<BasicBlock *>> _pathList){
        // copy pathList to this pathList
        for(const auto &path: _pathList){
            std::vector<BasicBlock*> newPath;

            for (const auto &basicBlock: path){
                newPath.push_back(basicBlock);
            }

            pathList.push_back(newPath);
        }
    }

    const std::vector<std::vector<BasicBlock *>> &getPathList() const {
        return pathList;
    }

    double getFitness() const {
        // for better score:
        // 1. code coverage should be max
        // 2. pathList size should be min
        std::set<BasicBlock *> allBlocksInPathList;
        for (const auto &path: pathList) {
            for (const auto &block: path) {
                allBlocksInPathList.insert(block);
            }
        }
        double pathListCoverage = ((double) allBlocksInPathList.size() / allBlocks.size()) * 100;
        // print path list


//        std::cout << "AA " << std::endl;
//        std::cout << "SIZE: " << pathList.size() << std::endl;

//        return  pathListCoverage / pathList.size();
        return  pathListCoverage;
    }

    Chromosome* crossover(Chromosome *other) const {
        // merge random number of path from this and other
        auto r1 = selectRandomNumberOfPaths(this);
        auto r2 = selectRandomNumberOfPaths(other);


        // merge r1 and r2
        std::vector<std::vector<BasicBlock *>> newPathList;
        for (const auto path: r1) {
            newPathList.push_back(path);
        }
        for (const auto path: r2) {
            newPathList.push_back(path);
        }

        return new Chromosome(newPathList);
    }

    void mutate() {
        // add random number of new paths or delete random number of paths
        int mutationType = randomInRange(0, 1);
        if (mutationType == 0) {
            // add new paths
            int newPathsCount = randomInRange(1, pathList.size() - 1);
            for (int i = 0; i < newPathsCount; i++) {
                std::vector<BasicBlock *> newPath = generateRandomPath(mainBasicBlock);
                pathList.push_back(newPath);
            }
        } else {
            // delete paths
            int deletePathsCount = randomInRange(1, pathList.size() - 1);
            for (int i = 0; i < deletePathsCount; i++) {
                int deletePathIndex = randomInRange(0, pathList.size() - 1);
                pathList.erase(pathList.begin() + deletePathIndex);
            }
        }
    }

    static std::vector<Chromosome> createInitialPopulation(int chromosomeCount, int chromosomeSize) {
        std::vector<Chromosome> population;
        for (int i = 0; i < chromosomeCount; i++) {
            std::vector<std::vector<BasicBlock *>> pathList;
            for (int j = 0; j < chromosomeSize; j++) {
                std::vector<BasicBlock *> path = generateRandomPath(mainBasicBlock);
                pathList.push_back(path);
            }
            population.emplace_back(pathList);
        }
        return population;
    }
};

class GeneticSearch {
private:
    std::vector<Chromosome> population;
    int crossoverRate;
    int mutationRate;
    int purgeRate;

    std::vector<Chromosome> getRandomCountOfSelectedPopulation(std::vector<Chromosome> selectedPopulation) {
        std::vector<Chromosome> result;
        for (const auto &p: population) {
            if (randomInRange(0, 1)) {
                result.push_back(p);
            }
        }
        return result;
    }

    static bool probabilityToHappen(int rate) {
        return randomInRange(0, 100) < rate;
    }

    Chromosome findBestScoreElement() {
        Chromosome bestScoreElement = population[0];
        for (auto &element: population) {
            if (element.getFitness() > bestScoreElement.getFitness()) {
                bestScoreElement = element;
            }
        }
        return bestScoreElement;
    }

    std::vector<Chromosome> selection() {
        return getRandomCountOfSelectedPopulation(population);
    }

    void crossover(std::vector<Chromosome> selectElements) {
        std::vector<Chromosome> newGeneration;
        for (int i = 0; i < randomInRange(0, selectElements.size()); i++) {
            int parent1Index = randomInRange(0, selectElements.size() - 1);
            int parent2Index = randomInRange(0, selectElements.size() - 1);
            Chromosome parent1 = selectElements[parent1Index];
            Chromosome parent2 = selectElements[parent2Index];
            Chromosome* child = parent1.crossover(&parent2);
            newGeneration.push_back(*child);
        }
        population.insert(population.end(), newGeneration.begin(), newGeneration.end());
    }

    void mutate(const std::vector<Chromosome>& selectElements) {
        for (auto &element: getRandomCountOfSelectedPopulation(selectElements)) {
            element.mutate();
        }
    }

    void purge() {
        // calculate average score
        int averageScore = 0;
        for (const auto &p: population) {
            averageScore += p.getFitness();
        }
        averageScore /= population.size();

        // purge
        std::vector<Chromosome> purgeList;
        for (const auto &p: population) {
            if (p.getFitness() < averageScore) {
                purgeList.push_back(p);
            }
        }
        population.erase(std::remove_if(population.begin(), population.end(),
                                        [&](const Chromosome &p) {
                                            for (const auto &purgeElement: purgeList) {
                                                if (purgeElement.getFitness() == p.getFitness()) {
                                                    return true;
                                                }
                                            }
                                            return false;
                                        }), population.end()
        );
    }

public:

    GeneticSearch(std::vector<Chromosome> population, int crossoverRate, int mutationRate, int purgeRate) :
            population(std::move(population)), crossoverRate(crossoverRate), mutationRate(mutationRate), purgeRate(purgeRate) {}

    Chromosome run(double goalScore, int maxGenerationNumber) {
        Chromosome bestScoreElement = findBestScoreElement();
        double globalMaxScore = bestScoreElement.getFitness();
        llvm::outs() << "Best founded of initial generation, Score: " << globalMaxScore << "\n";

        int generationNumber;
        for (generationNumber = 1; bestScoreElement.getFitness() != goalScore; generationNumber++) {

            if (generationNumber > maxGenerationNumber) {
                llvm::outs() << "Maximum generation number exceeded\n";
                return bestScoreElement;
            }

            if (bestScoreElement.getFitness() > globalMaxScore) {
                globalMaxScore = bestScoreElement.getFitness();
                llvm::outs() << "New Best founded in generation(" << generationNumber << ") Score: " << globalMaxScore
                             << "\n";
                llvm::outs() << "Current population : " << population.size() << "\n";
            }

//            llvm::outs() << "start of selection" << "\n";
            std::vector<Chromosome> selectElements = selection();

            if (probabilityToHappen(crossoverRate)) crossover(selectElements);

            if (probabilityToHappen(mutationRate)) mutate(selectElements);
//            llvm::outs() << "end of selection" << "\n";

            if (probabilityToHappen(purgeRate)) purge();

            bestScoreElement = findBestScoreElement();
        }
        llvm::outs() << "Solution found in generation(" << generationNumber << ")\n";
        return bestScoreElement;
    }
};


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
//            mainBasicBlock = &F.getEntryBlock();
            mainBasicBlock = dyn_cast<BasicBlock>(F.begin());
            break;
        }
    }

//    auto pop = Chromosome::createInitialPopulation(2, 2);
//    for(auto p : pop){
//        llvm::outs() << "----------------------------\n";
//        llvm::outs() << (int) (p.getFitness()) << "\n";
//            for(auto b : p.getPathList()){
//                for(auto i : b){
//                    llvm::outs() << i->getName() << " ";
//                }
//                llvm::outs() << "\n";
//            }
//        llvm::outs() << "----------------------------\n";
//    }



    GeneticSearch geneticSearch(Chromosome::createInitialPopulation(100, 5),
                                85,
                                35,
                                30
    );
    Chromosome bestChromosome = geneticSearch.run(100, 5000);

    llvm::outs() << "Best chromosome: " << bestChromosome.getFitness() << "\n";
    llvm::outs() << "Size: " << bestChromosome.getPathList().size() << "\n";


    for(auto path : bestChromosome.getPathList()){
        printRange(path);
    }


//    for (auto &function: *M) {
//        if (function.getName().str() == "main") {
//            mainBasicBlock = dyn_cast<BasicBlock>(function.begin());
//
//            auto path = generateRandomPath(mainBasicBlock);
////            printRange(path);
//            std::cout << "Path Coverage: " << ((double) path.size() / allBlocks.size()) * 100 << "\n";
//
//
//            llvm::outs() << "************** Path **************" << "\n";
//            for (auto &basicBlock: path) {
//                llvm::outs() << getSimpleNodeName(basicBlock) << "\n";
//            }
//        }
//    }


//    llvm::outs() << "======== Analysis Result =========" << "\n";
//
//    // print data of rangeVariableMap
//    for (auto &it: rangeVariableMap) {
//        std::cout << it.first << ": ";
//        for (auto &i: it.second) {
//            std::cout << i << " ";
//        }
//        std::cout << "\n";
//    }
//
//    llvm::outs() << "======= Random Test Output =======" << "\n";
//
//    for (auto &it: rangeVariableMap) {
//        unsigned rnd = randomInRange(0, it.second.size() - 1);
//
//        // cast it.second to vector<int>
//        std::vector<int> v(it.second.begin(), it.second.end());
//
//        llvm::outs() << it.first << ": " << v[rnd] << "\n";
//    }

    return 0;
}

std::shared_ptr<std::tuple<CmpInst::Predicate, std::string, int>> getCmpData(BasicBlock *basicBlock) {

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

//            return new std::tuple<CmpInst::Predicate, std::string, int>(cmpType, cmpVariableName, cmpImmediateValue);
            return std::make_shared<std::tuple<CmpInst::Predicate, std::string, int>>(cmpType, cmpVariableName,
                                                                                      cmpImmediateValue);
        }
    }
    return nullptr;
}

void analyzeVariableRange(std::shared_ptr<std::tuple<CmpInst::Predicate, std::string, int>> cmpData) {
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

void printRange(const std::vector<BasicBlock *> &path) {
    if (VERBOSE_LOG) {
        llvm::outs() << "----------- Conditions -----------" << "\n";
    }
    for (BasicBlock *bb: path) {

        llvm::outs() << getSimpleNodeName(bb) << "\n";

        // get bb previous basic block
        BasicBlock *prevBB = bb->getSinglePredecessor();
        auto cmpData = getCmpData(prevBB);
        if (prevBB && cmpData != nullptr) {
            // get all elements of cmpData
            CmpInst::Predicate cmpType;
            std::string cmpVariableName;
            int cmpImmediateValue;
            std::tie(cmpType, cmpVariableName, cmpImmediateValue) = *cmpData;

            // if we are in false branch complement the cmp type
            if (prevBB->getTerminator()->getNumSuccessors() == 2 && prevBB->getTerminator()->getSuccessor(1) == bb) {
                auto complementCmpType = getComplementType(cmpType);
                std::get<0>(*cmpData) = complementCmpType;
            }
            analyzeVariableRange(cmpData);

            if (VERBOSE_LOG) {
                llvm::outs() << getSimpleNodeName(bb) << "\n";
                printCmpData(cmpData);
                llvm::outs() << "----------------------------------" << "\n";
            }

            analyzeAssignmentData(bb);
        }
    }
}

std::vector<BasicBlock *> generateRandomPath(BasicBlock *basicBlock) {
    std::vector<BasicBlock *> path = {basicBlock};
    Instruction * terminatorInst = basicBlock->getTerminator();
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

void printCmpData(std::shared_ptr<std::tuple<CmpInst::Predicate, std::string, int>> cmpData) {
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


