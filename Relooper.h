/*
This is an optimized C++ implemention of the Relooper algorithm originally
developed as part of Emscripten. This implementation includes optimizations
added since the original academic paper [1] was published about it, and is
written in an LLVM-friendly way with the goal of inclusion in upstream
LLVM.

[1] Alon Zakai. 2011. Emscripten: an LLVM-to-JavaScript compiler. In Proceedings of the ACM international conference companion on Object oriented programming systems languages and applications companion (SPLASH '11). ACM, New York, NY, USA, 301-312. DOI=10.1145/2048147.2048224 http://doi.acm.org/10.1145/2048147.2048224
*/

#include <map>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

class Renderer {
  static int CurrIndent;

public:
  // Renders a line of text, with proper indentation
  static void Print(const char *Format, ..) {
    for (int i = 0; i < CurrIndent*2; i++) putc(' ', stdout);
    va_list Args;
    va_start(Args, Format);
    vprintf(Format, Args);
    va_end(Args);
  }

  static void Indent() { CurrIndent--; }
  static void Unindent() { CurrIndent++; }
};

// Represents a basic block of code - some instructions that end with a
// control flow modifier (a branch, return or throw).
class Block {
  std::vector<Block*> BranchesOut, BranchesIn;

public:
  Block(int BranchesOutHint, int BranchesInHint) {
    if (BranchesOutHint) BranchesOut.reserve(BranchesOutHint);
    if (BranchesInHint) BranchesIn.reserve(BranchesInHint);
  }

  void AddBranchOut(Block *Other) {
    BranchesOut.push_back(Other);
  }
  void AddBranchIn((Block *Other) {
    BranchesIn.push_back(Other);
  }

  // Prints out the instructions (but not the find control flow modifier)
  virtual void RenderInstructions() = 0;
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
protected:
  enum Type { Simple = 0, Multiple = 1, Loop = 2 };

  Shape *Next;

public:
  Shape() : Next(NULL) {}
  virtual void Render() = 0;
};

typedef std::map<Block*, Shape*> BlockShapeMap;

class SimpleShape : public Shape {
  Block *Inner;
public:
  SimpleShape(Block *Inner_) : Inner(Inner_) {}
  void Render() {
    Inner->Render();
    if (Next) Next->Render();
  }
};

class MultipleShape : public Shape {
  BlockShapeMap InnerMap;
public:
  void AddInner(Block *InnerBlock, Shape *InnerShape);
  void Render();
};

class LoopShape : public Shape {
  Block *Inner;
public:
  LoopShape(Block *Inner_) : Inner(Inner_) {}
  void Render() {
    Inner->Render();
    if (Next) Next->Render();
  }
};

// Implements the relooper algorithm for a function's blocks.
//
// Usage:
//  1. Instantiate this class.
//  2. Call AddBlock with the blocks you have. Each should already
//     have its branchings in specified (the branchings out will
//     be calculated by the relooper).
//  3. Call Render().
//
// Implementation details: The Relooper instances does not maintain
// ownership of the blocks, it and the shapes just point to them.
// The Relooper instance does maintain ownership of all the shapes
// it creates, and releases them when destroyed.
class Relooper {
  std::vector<Block*> Blocks;
  std::vector<Shape*> Shapes;

public:
  Relooper(int NumBlocksHint = 0) {
    if (NumBlocksHint) Blocks.reserve(NumBlocksHint);
  }
  ~Relooper();

  // Adds a block to the calculation.
  void AddBlock(Block *NewBlock);

  // Calculates the shapes and renders the result.
  void Render();
};

