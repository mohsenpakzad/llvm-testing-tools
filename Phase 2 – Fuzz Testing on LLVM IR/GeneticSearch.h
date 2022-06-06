#ifndef PHASE_2__FUZZ_TESTING_ON_LLVM_IR_GENETICSEARCH_H
#define PHASE_2__FUZZ_TESTING_ON_LLVM_IR_GENETICSEARCH_H

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

#include "RandomPath.h"
#include "Utils.h"

using namespace llvm;

extern BasicBlock *mainBasicBlock;
extern std::set<BasicBlock *> allBlocks;

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
    Chromosome(std::vector<std::vector<BasicBlock *>> _pathList) {
        // copy pathList to this pathList
        for (const auto &path: _pathList) {
            std::vector<BasicBlock *> newPath;

            for (const auto &basicBlock: path) {
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

        return pathListCoverage + allBlocks.size() - pathList.size();
//        return  pathListCoverage;
    }

    Chromosome *crossover(Chromosome *other) const {
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
            int newPathsCount = randomInRange(0, pathList.size());
            for (int i = 0; i < newPathsCount; i++) {
                std::vector<BasicBlock *> newPath = generateRandomPath(mainBasicBlock);
                pathList.push_back(newPath);
            }
        } else {
            // delete paths
            int deletePathsCount = randomInRange(0, pathList.size());
            for (int i = 0; i < deletePathsCount; i++) {
                int deletePathIndex = randomInRange(0, pathList.size());
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
        for (int i = 0; i < randomInRange(0, selectElements.size() - 1); i++) {
            int parent1Index = randomInRange(0, selectElements.size() - 1);
            int parent2Index = randomInRange(0, selectElements.size() - 1);
            Chromosome parent1 = selectElements[parent1Index];
            Chromosome parent2 = selectElements[parent2Index];
            Chromosome *child = parent1.crossover(&parent2);
            newGeneration.push_back(*child);
        }
        population.insert(population.end(), newGeneration.begin(), newGeneration.end());
    }

    void mutate(const std::vector<Chromosome> selectElements) {
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
            population(std::move(population)), crossoverRate(crossoverRate), mutationRate(mutationRate),
            purgeRate(purgeRate) {}

    Chromosome run(double goalScore, int maxGenerationNumber) {
        Chromosome bestScoreElement = findBestScoreElement();
        double globalMaxScore = bestScoreElement.getFitness();
        llvm::outs() << "Best founded of initial generation, Score: " << (int) globalMaxScore << "\n";

        int generationNumber;
        for (generationNumber = 1; bestScoreElement.getFitness() != goalScore; generationNumber++) {

            llvm::outs() << "Current population : " << population.size() << "\n";

            if (generationNumber > maxGenerationNumber) {
                llvm::outs() << "Maximum generation number exceeded\n";
                return bestScoreElement;
            }

            if (bestScoreElement.getFitness() > globalMaxScore) {
                globalMaxScore = bestScoreElement.getFitness();
                llvm::outs() << "New Best founded in generation(" << generationNumber << ") Score: "
                             << (int) globalMaxScore << "\n";
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


#endif //PHASE_2__FUZZ_TESTING_ON_LLVM_IR_GENETICSEARCH_H
