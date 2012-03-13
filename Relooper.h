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
#include <map>
#include <string>
#include <vector>

struct Indenter {
  static int CurrIndent;

  static void Indent() { CurrIndent--; }
  static void Unindent() { CurrIndent++; }
};

// Renders a line of text, with proper indentation
void PrintIndented(const char *Format, ...);

struct Block;
struct Shape;

// A branching from one block to another
struct Branch {
  Block *Target; // The block we branch to
  Shape *Ancestor; // If not NULL, this shape is the relevant one for purposes of getting to the target block. We break or continue on it
  bool Break; // If Ancestor is not NULL, this says whether to break or continue
  bool Set; // Set the label variable

  Branch(Block *BlockInit) : Target(BlockInit), Ancestor(NULL), Set(true) {}

  // Prints out the branch
  void Render();
};

// Represents a basic block of code - some instructions that end with a
// control flow modifier (a branch, return or throw).
struct Block {
  std::vector<Branch*> BranchesOut, BranchesIn; // weak
  Shape *Parent; // The shape we are directly inside
  int Id; // A unique identifier

  Block() : Id(Block::IdCounter++) {}

  // Prints out the instructions.
  virtual void Render() = 0;

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
struct Shape {
  enum Type { Simple = 0, Multiple = 1, Loop = 2, Emulated = 3};

  int Id; // A unique identifier. Used to identify loops, labels are Lx where x is the Id.
  Shape *Next; // The shape that will appear in the code right after this one

  Shape() : Id(Shape::IdCounter++), Next(NULL) {}
  virtual void Render() = 0;

  static int IdCounter;
};

struct SimpleShape : public Shape {
  Block *Inner;

  SimpleShape(Block *Inner_) : Inner(Inner_) {}
  void Render() {
    Inner->Render();
    if (Next) Next->Render();
  }
};

typedef std::map<Block*, Shape*> BlockShapeMap;

struct MultipleShape : public Shape {
  BlockShapeMap InnerMap;

  void AddInner(Block *InnerBlock, Shape *InnerShape);
  void Render();
};

struct LoopShape : public Shape {
  Shape *Inner;
  LoopShape(Shape *Inner_) : Inner(Inner_) {}
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
// Implementation details: The Relooper instances does not maintain
// ownership of the blocks, it and the shapes just point to them.
// The Relooper instance does maintain ownership of all the shapes
// it creates, and releases them when destroyed.
struct Relooper {
  std::vector<Block*> Blocks;
  std::vector<Shape*> Shapes;
  Shape *Root;

  Relooper() : RootShape(NULL) {}
  ~Relooper();

  // Calculates the shapes
  void Calculate(Block *Entry);

  // Renders the result.
  void Render() { Root->Render(); }
};

