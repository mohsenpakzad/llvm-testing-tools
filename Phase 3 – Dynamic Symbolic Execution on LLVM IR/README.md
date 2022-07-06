# Software Testing Project 

## Phase 3 / Dynamic Symbolic Execution
---

[`Mohsen Pakzad`](https://github.com/mohsenpakzad)
[`Alireza Bozorgomid`](https://github.com/xbozorg)

---
There are several goals for this assignment:
- Designing a simple dynamic symbolic execution tool.
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

## DSE Tester Compilation
```sh
clang++-10  -o FuzzTester FuzzTester.cpp `llvm-config-10 --cxxflags` `llvm-config-10 --ldflags` `llvm-config-10 --libs` -lpthread -lncurses -ldl
 ./FuzzTester "$1"
```

---

## Design Description
**The purpose of this phase of the project is to use LLVM API in C++ to analyze the LLVM IR codes and use dynamic symbolic execution rules to traverse conditions in IR codes to reach error/bug prone statements.
Ending Condition : When we negate last condition and it doesn't makes a new path.**

---


## Input example :
#### `test.c`
```c
int main() {
    int a1;
    if (a1 >= 1000) {
        if(a1 <= 1500){
            if(a1 == 1401){
                return a1;
            }
        }
    } 
    return a1;
}
```
## Output example:
#### `./FuzzTester test.ll`
```sh
************** Input Argument(s) ***************
a1 = -171496
*************** Navigated Path *****************
entry
if.end6
return
************** Input Argument(s) ***************
a1 = 165395
*************** Navigated Path *****************
entry
if.then
if.end5
if.end6
return
************** Input Argument(s) ***************
a1 = 1378
*************** Navigated Path *****************
entry
if.then
if.then2
if.end
if.end5
if.end6
return
************** Input Argument(s) ***************
a1 = 1401
*************** Navigated Path *****************
entry
if.then
if.then2
if.then4
return
****************** Coverage ********************
100%
```
---
### `Solver` Class
```c++
Solver(std::vector<ICmpInst *> comparisonInstructions, int minRange, int maxRange): comparisonInstructions(std::move(comparisonInstructions)), minRange(minRange), maxRange(maxRange) {}
```
Basic solver for DSE conditions.
### `applyComparisons`
```c++
void applyComparisons() {}
```
Apply all comparisons with using of `applyCmpInstToVariablesRange` 

### `applyCmpInstToVariablesRange`
```c++
void applyCmpInstToVariablesRange(CmpInst::Predicate cmpPredicate,const std::pair<std::string, std::set<int>> &opCmp1Range,const std::pair<std::string,std::set<int>> &opCmp2Range) {
```
Apply integer range of variables in a single comparison.
### `PathNavigator` Class
```c++
PathNavigator(BasicBlock *entryBlock, std::map<std::string, int> argumentsMap): entryBlock(entryBlock), variablesMap(std::move(argumentsMap)) {}
```
Traverse through paths , use random inputs and runs the code with the initial values.

### `negateCmpPredicate`
```c++
CmpInst::Predicate negateCmpPredicate(CmpInst::Predicate predicate) {}
```
Gets a comparison and negate it.


