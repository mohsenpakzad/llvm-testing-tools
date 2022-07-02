# Software Testing Project 

## _Phase 2 / Fuzz Testing on LLVM IR_
---

[`Mohsen Pakzad`](https://github.com/mohsenpakzad)
[`Alireza Bozorgomid`](https://github.com/xbozorg)

---
There are several goals for this assignment:
- Designing a simple fuzz testing tool.
- Gaining exposure to LLVM in general and the LLVM IR which is the intermediate representation
used by LLVM.
- Using LLVM to perform a sample testing.
  
  
---  
  
  
## Test Compilation , CFG pdf


```sh
clang-10 -fno-discard-value-names -emit-llvm -S -o test1.ll test1.c
opt-10 -dot-cfg test1.ll
mv .main.dot test1.dot
./allfigs2pdf
```

## Fuzz Tester Compilation
```sh
clang++-10  -o FuzzTester FuzzTester.cpp `llvm-config-10 --cxxflags` `llvm-config-10 --ldflags` `llvm-config-10 --libs` -lpthread -lncurses -ldl
 ./FuzzTester sample-codes/test1.ll
```

---

## Design Description
**The purpose of this phase of the project is to use LLVM API in C++ to analyze the LLVM IR codes and generate multiple and different seeds from an initial seed to pass through maximum number of paths, in such a way that all this seeds together obtains maximum test coverage.**


**For this purpose we use `Genetic Algorithm` ,
We have a `Chromosome` class that has a pathList ( vector of BasicBlock vectors ) 
We have a `Population` that is a vector of `Chromosomes`
We can run genetic search with custom goalScore and number of generations
We have an initial population ( `seed` ) and use `crossover` , `mutation` and `purge` to generate new seeds and calculates fitness ( `test coverage` ) of these seeds**

---


## Input example :
#### `test.c`
```c
int main() {
   int a, c = 0;

   if (a > 0)
     c += 10;
   else 
     c += 15;
}
```
## Output example:
#### `./FuzzTester test.ll`
```sh
All blocks:4
Best founded of initial generation, Score: 99
Current population : 100
Current population : 100
Current population : 102
Current population : 113
New Best founded in generation(4) Score: 100
Current population : 113
Current population : 117
Current population : 126
Current population : 135
New Best founded in generation(7) Score: 101
Current population : 135
Current population : 139
Current population : 149
New Best founded in generation(9) Score: 102
Current population : 149
Current population : 140
Current population : 152
Current population : 154
Current population : 169
Current population : 172
Current population : 149
Current population : 157
Current population : 158
Current population : 158
Current population : 165
Current population : 173
Current population : 180
Current population : 200
Current population : 205
Current population : 217
Current population : 227
Current population : 245
Current population : 255
Current population : 243
Current population : 257
Current population : 260
Current population : 261
Current population : 274
Current population : 274
Current population : 290
Current population : 316
Current population : 344
Current population : 352
Current population : 366
Current population : 403
Current population : 417
Current population : 427
Current population : 444
Current population : 444
Current population : 423
Current population : 423
Current population : 438
Current population : 459
Current population : 442
Current population : 464
Current population : 470
Current population : 481
Maximum generation number exceeded
----------- Conditions -----------
if.then
a > 0
----------------------------------
************** Path **************
entry
if.then
if.end
======== Analysis Result =========
a: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
======= Random Test Output =======
a: 13
----------- Conditions -----------
if.else
a <= 0
----------------------------------
************** Path **************
entry
if.else
if.end
======== Analysis Result =========
a: -20 -19 -18 -17 -16 -15 -14 -13 -12 -11 -10 -9 -8 -7 -6 -5 -4 -3 -2 -1 0
======= Random Test Output =======
a: -13
```
---
## `Chromosome` Class
```c++
std::vector<std::vector<BasicBlock *>> pathList;
```
Each `chromosome` has a `pathList` (a vector of paths , and each path is a vector of basic blocks) 

```c++
std::vector<std::vector<BasicBlock *>> selectRandomNumberOfPaths(const Chromosome *chromosome);
```
Iterates through `pathList` and randomly select paths and return those paths as a `newPathList`

```c++
Chromosome(std::vector<std::vector<BasicBlock *>> _pathList);
```
Constructor

```c++
std::vector<std::vector<BasicBlock *>> &getPathList() const;
```
Getter for `pathList`

```c++
double getFitness() const;
```
Calculates `fitness` of genetic algorithm ( `pathList Coverage` ) with a formula that use
- Number of blocks in path list
- Total blocks in code

and returns it.

```c++
Chromosome* crossover(Chromosome *other) const;
```
Select random number of paths of two `parent chromosomes` and combines them to generate a new `offspring chromosome`

```c++
void mutate()
```
Add or Remove random paths to/from a chromosome

```c++
static std::vector<Chromosome> createInitialPopulation(int chromosomeCount, int chromosomeSize);
```
Gets number of chromosomes to generate and size of those chromosomes and generates an initial population of chromosomes

---

## `GeneticSearch` Class

```c++
std::vector<Chromosome> population;
```
Population of genetic algorithm . Vector of Chromosomes 

```c++
int crossoverRate;
```
Probability that crossover happens

```c++
int mutationRate;
```
Probability that mutation happens
```c++
int purgeRate;
```
Probability that purge happens
```c++
std::vector<Chromosome> getRandomCountOfSelectedPopulation(std::vector<Chromosome> selectedPopulation);
```
Get Random Count Of Selected Population
```c++
static bool probabilityToHappen(int rate);
```

```c++
Chromosome findBestScoreElement();
```
Find best score element of population
```c++
std::vector<Chromosome> selection();
```
Randomly selects elements from population
```c++
void crossover(std::vector<Chromosome> selectElements);
```
Crossover random number of selected population
```c++
void mutate(const std::vector<Chromosome>& selectElements);
```
Mutate random number of selected population
```c++
void purge();
```
Purge random number of selected population
```c++
Chromosome run(double goalScore, int maxGenerationNumber);
```
Run genetic search


---