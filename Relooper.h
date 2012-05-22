/*
This is an optimized C++ implemention of the Relooper algorithm originally
developed as part of Emscripten. This implementation includes optimizations
added since the original academic paper [1] was published about it, and is
written in an LLVM-friendly way with the goal of inclusion in upstream
LLVM.

[1] Alon Zakai. 2011. Emscripten: an LLVM-to-JavaScript compiler. In Proceedings of the ACM international conference companion on Object oriented programming systems languages and applications companion (SPLASH '11). ACM, New York, NY, USA, 301-312. DOI=10.1145/2048147.2048224 http://doi.acm.org/10.1145/2048147.2048224
*/

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus

#include <map>
#include <string>
#include <vector>
#include <set>

struct Indenter {
  static int CurrIndent;

  static void Indent() { CurrIndent++; }
  static void Unindent() { CurrIndent--; }
};

// Renders a line of text, with proper indentation
void PrintIndented(const char *Format, ...);
void PutIndented(const char *String);

struct Block;
struct Shape;

// Info about a branching from one block to another
struct Branch {
  enum FlowType {
    Direct = 0, // We will directly reach the right location through other means, no need for continue or break
    Break = 1,
    Continue = 2
  };
  Shape *Ancestor; // If not NULL, this shape is the relevant one for purposes of getting to the target block. We break or continue on it
  Branch::FlowType Type; // If Ancestor is not NULL, this says whether to break or continue
  bool Set; // Set the label variable
  char *Condition; // The condition for which we branch. For example, "== 1". Conditions are checked one by one, and the last is ignored (always taken)

  Branch(char *ConditionInit) : Ancestor(NULL), Set(true), Condition(ConditionInit) {}

  // Prints out the branch
  void Render(Block *Target); // We do not store the target permanently to save memory, it is only used here
};

typedef std::map<Block*, Branch*> BlockBranchMap;

// Represents a basic block of code - some instructions that end with a
// control flow modifier (a branch, return or throw).
class Block {
public:
  // Branches become processed after we finish the shape relevant to them. For example,
  // when we recreate a loop, branches to the loop start become continues and are now
  // processed. When we calculate what shape to generate from a set of blocks, we ignore
  // processed branches.
  // Blocks own the Branch objects they use, and destroy them when done.
  BlockBranchMap BranchesOut;
  BlockBranchMap BranchesIn;
  BlockBranchMap ProcessedBranchesOut;
  BlockBranchMap ProcessedBranchesIn;
  Shape *Parent; // The shape we are directly inside
  int Id; // A unique identifier

  char *Code; // The string representation of the code in this block. Owning pointer (we copy the input)
  char *Condition; // The variable on which we branch. Null if no branches from this block. Owning pointer (we copy the input)
  Block *DefaultTarget; // If specified, the block we branch to without checking the condition, if none of the other conditions held

  Block(const char *CodeInit, const char *ConditionInit);
  ~Block();

  void AddBranchTo(Block *Target, char *Condition);

  // Prints out the instructions code and branchings
  void Render();

private:
  static int IdCounter;
};

// Represents a structured control flow shape, one of
//
//  Simple: No control flow at all, just instructions. If several
//          blocks, then 
//
//  Multiple: A shape with more than one entry. If the next block to
//            be entered is among them, we run it and continue to
//            the next shape, otherwise we continue immediately to the
//            next shape.
//
//  Loop: An infinite loop.
//
//  Emulated: Control flow is managed by a switch in a loop. This
//            is necessary in some cases, for example when control
//            flow is not known until runtime (indirect branches,
//            setjmp returns, etc.)
//
class Shape {
public:
  enum Type { Simple = 0, Multiple = 1, Loop = 2, Emulated = 3};

  int Id; // A unique identifier. Used to identify loops, labels are Lx where x is the Id.
  Shape *Next; // The shape that will appear in the code right after this one

  Shape() : Id(Shape::IdCounter++), Next(NULL) {}
  virtual ~Shape() {}

  virtual void Render() = 0;

private:
  static int IdCounter;
};

struct SimpleShape : public Shape {
  Block *Inner;

  SimpleShape() : Inner(NULL) {}
  void Render() {
    Inner->Render();
    if (Next) Next->Render();
  }
};

typedef std::map<Block*, Shape*> BlockShapeMap;

struct MultipleShape : public Shape {
  BlockShapeMap InnerMap;

  void Render();
};

struct LoopShape : public Shape {
  Shape *Inner;
  LoopShape() : Inner(NULL) {}
  void Render();
};

struct EmulatedShape : public Shape {
  std::vector<Block*> Blocks;
  void Render();
};

// Implements the relooper algorithm for a function's blocks.
//
// Usage:
//  1. Instantiate this struct.
//  2. Call AddBlock with the blocks you have. Each should already
//     have its branchings in specified (the branchings out will
//     be calculated by the relooper).
//  3. Call Render().
//
// Implementation details: The Relooper instance has
// ownership of the blocks and shapes, and frees them when done.
struct Relooper {
  std::vector<Block*> Blocks;
  std::vector<Shape*> Shapes;
  Shape *Root;

  Relooper();
  ~Relooper();

  void AddBlock(Block *New);

  // Calculates the shapes
  void Calculate(Block *Entry);

  // Renders the result.
  void Render() { Root->Render(); }
};

typedef std::set<Block*> BlockSet;
typedef std::map<Block*, BlockSet> BlockBlockSetMap;

struct Debugging {
  static bool On;
  static void Dump(BlockSet &Blocks, const char *prefix=NULL);
};

void PrintDebug(const char *Format, ...);

#endif // __cplusplus

// C API - useful for binding to other languages

#ifdef _WIN32
  #ifdef RELOOPERDLL_EXPORTS
    #define RELOOPERDLL_API __declspec(dllexport)
  #else
    #define RELOOPERDLL_API __declspec(dllimport)
  #endif
#else
  #define RELOOPERDLL_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

RELOOPERDLL_API void *rl_new_block(char *text, char *check);
RELOOPERDLL_API void  rl_delete_block(void *block);
RELOOPERDLL_API void  rl_block_add_branch_to(void *from, void *to, char *condition);
RELOOPERDLL_API void *rl_new_relooper();
RELOOPERDLL_API void  rl_delete_relooper(void *relooper);
RELOOPERDLL_API void  rl_relooper_add_block(void *relooper, void *block);
RELOOPERDLL_API void  rl_relooper_calculate(void *relooper, void *entry);
RELOOPERDLL_API void  rl_relooper_render(void *relooper);

#ifdef __cplusplus
}
#endif

