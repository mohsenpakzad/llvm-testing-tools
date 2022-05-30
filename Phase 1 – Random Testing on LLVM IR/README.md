# Software Testing Project 

## _Phase 1 / Random Testing on LLVM IR_
---

[`Mohsen Pakzad`](https://github.com/mohsenpakzad)
[`Alireza Bozorgomid`](https://github.com/xbozorg)

---
There are several goals for this assignment:
- Designing a simple random testing tool.
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

## Random Tester Compilation
```sh
clang++-10  -o RandomPath RandomPath.cpp `llvm-config-10 --cxxflags` `llvm-config-10 --ldflags` `llvm-config-10 --libs` -lpthread -lncurses -ldl
 ./RandomPath test1.ll
```

---

## Design Description
**The purpose of this phase of the project is to use LLVM API in C++ to analyze the LLVM IR codes and obtain the ranges of each variable used in the code based on comparative and assignment instructions.**


**For this purpose we use the `generateRandomPath` function , which randomly traverses a path from the beginning to the end of the code.
In such a way that in each case of comparative instruction, according to it's condition , we apply it to the variables range.
And then along the way, if we face an assignment instruction, we apply it to the corresponding variable range.**

**Finally , we generate a random number in the range of these numbers that we found along the way we passed, as the test case and the result of the analysis.**


---

## Supported Features

###### Comparison Instructions:
- Any simple operation with a variable and a constant value in any order. such as :`a > 10` , ` -3 <= b` , `c != 5` , ...
- Support all comparative operators :`( == , != , > , < , >= , <= )`

###### Assignment Instructions:
* Any simple assignment with :
    * one exact value : ` a = 10 `
    * operations on two exact value : ` a = 10 - 5 `
    * operations on one exact and one variable in any order : ` a = b + 10 ` , `x = 3 * y ` , ` a /= 2 `
    * operations on two variables : ` a = b + c ` , ` x = y / z `

* Supported Operations : ` + , - , * , / `

###### Extras:
* Nested `if` and `else` blocks
* Support `else if (elif)` statements
* Ability to generate verbose log along the path
* Configurable `MIN_RANGE` and `MAX_RANGE`
* Ability to show exact range of each variables along with the output test cases
---

## Input example :
#### `test.c`
```c
int main() {
  int a, b, x = 0;
  if (a > 0) {
    x -= 5;
  } else {
    x = 2;
  }
  if (b > 0) {
    x = 3;
  } else {
    x += 51;
  }
}
```
## Output example:
#### `./RandomPath test.c`
```sh
----------- Conditions -----------
if.then
a > 0
----------------------------------
if.else3
b <= 0
----------------------------------
************** Path **************
entry
if.then
if.end
if.else3
if.end4
======== Analysis Result =========
a: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
b: -20 -19 -18 -17 -16 -15 -14 -13 -12 -11 -10 -9 -8 -7 -6 -5 -4 -3 -2 -1 0
x: 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66
======= Random Test Output =======
a: 2
b: -17
x: 62
```
---
## Global Variables



```c++
bool VERBOSE_LOG = true;
```
`true` -> Print exclusive verbose log.
```c++
const int MIN_RANGE = -20;
const int MAX_RANGE = 20;
```
Specify range of the variables for analysis.
```c++
std::map<std::string, std::set<int>> rangeVariableMap;
```
Data structure that represents range of variables based on `MIN_RANGE` and `MAX_RANGE`.

---

## Functions
```c++
std::vector<BasicBlock *> generateRandomPath(BasicBlock *basicBlock);
```
Generate random path from initial basic block.
```c++
std::string getSimpleNodeName(const Value *node);
```
Get a value and returns it's name.
```c++
std::tuple<CmpInst::Predicate, std::string, int> *getCmpData(BasicBlock *basicBlock);

```

Returns comparison data in basic block in form of `(comparison operator , variable name , comparison value)`
```c++
void analyzeVariableRange(std::tuple<CmpInst::Predicate, std::string, int> *cmpData);
```
Get comparison data and apply it on variable and save it in `rangeVariableMap`.
```c++
std::set<int>rangeOperation(const std::set<int> &range1, const std::set<int> &range2, Instruction::BinaryOps binaryOps);
```
Apply binary operation on two ranges.
```c++
std::set<int> rangeOperation(const std::set<int> &range, int absoluteValue, Instruction::BinaryOps binaryOps);
```
Apply binary operation on a range and a absolute value.

```c++
int exactValueOperation(int e1, int e2, Instruction::BinaryOps binaryOps);
```
Apply binary operation on exact value.

```c++
int randomInRange(int startOfRange, int endOfRange);
```
Create a random number in the range.
```c++
CmpInst::Predicate getComplementType(CmpInst::Predicate predicate);
```
Complements type of a comparison instruction operator.
```c++
std::string getCmpTypeString(CmpInst::Predicate predicate);
```
Create string representation of comparison instruction operator.
```c++
void printCmpData(std::tuple<CmpInst::Predicate, std::string, int> *cmpData);
```
Prints comparison data in friendly format.


```c++
std::set<int> getOrCreateVariableRange(const std::string& variableName);
```
Get variable range based on `rangeVariableMap` if `variableName` exists in map otherwise it creates initial range based on `MIN_RANGE` and `MAX_RANGE` for variable and returns it.

```c++
void analyzeAssignmentData(BasicBlock *BB);
```
Analyze basic block assignment instructions and apply it on `rangeVariableMap`

---