
#include "Relooper.h"


void PrintIndented(const char *Format, ...) {
  for (int i = 0; i < Indenter::CurrIndent*2; i++) putc(' ', stdout);
  va_list Args;
  va_start(Args, Format);
  vprintf(Format, Args);
  va_end(Args);
}

// Indenter

int Indenter::CurrIndent = 0;

// Branch

void Branch::Render() {
  if (Set) PrintIndented("label = %d;\n", Target->Id);
  if (Ancestor) {
    PrintIndented("%s L%d;\n", Break ? "break" : "continue", Ancestor->Id);
  }
}

// Block

int Block::IdCounter = 0;

// Shape

int Shape::IdCounter = 0;

// MultipleShape

void MultipleShape::Render() {
  bool First = true;
  for (BlockShapeMap::iterator iter = InnerMap.begin(); iter != InnerMap.end(); iter++) {
    PrintIndented("%s if (label == %d) {\n", First ? "" : "else ", iter->first);
    First = false;
    Indenter::Indent();
    iter->second->Render();
    Indenter::Unindent();
    PrintIndented("}\n");
  }
  if (Next) Next->Render();
};

// LoopShape

void LoopShape::Render() {
  PrintIndented("while(1) {\n");
  Indenter::Indent();
  Inner->Render();
  Indenter::Unindent();
  PrintIndented("}\n");
  if (Next) Next->Render();
};

// EmulatedShape

void EmulatedShape::Render() {
  PrintIndented("while(1) {\n");
  Indenter::Indent();
  PrintIndented("switch(label) {\n");
  Indenter::Indent();
  for (int i = 0; i < Blocks.size(); i++) {
    Block *Curr = Blocks[i];
    PrintIndented("case %d: {\n", Curr->Id);
    Indenter::Indent();
    Curr->Render();
    PrintIndented("break;\n");
    Indenter::Unindent();
    PrintIndented("}\n");
  }
  Indenter::Unindent();
  PrintIndented("}\n");
  Indenter::Unindent();
  PrintIndented("}\n");
  if (Next) Next->Render();
};

// Relooper

Relooper::~Relooper() {
  // Delete shapes..
}

void Relooper::Calculate() {
  Shapes.reserve(Blocks.size()/2); // vague heuristic, better than nothing

  // Add incoming branches
  for (int i = 0; i < Blocks.size(); i++) {
    Block *Curr = Blocks[i];
    for (int j = 0; j < Curr->BranchesOut.size(); j++) {
      Curr->BranchesOut[j]->Target->BranchesIn.push_back(new Branch(Curr)); // XXX leaky
    }
  }

  // Recursively process the graph

  typedef std::set<Block*> BlockSet;
  typedef std::vector<Block*> BlockVec;
  typedef std::vector<BlockVec> BlockBlockVec;

  struct Recursor {
    Relooper *Parent;
    Recursor(Relooper *ParentInit) : Parent(ParentInit) {}

    // Add a shape to the list of shapes in this Relooper calculation
    void Notice(Shape *New) { Parent->Shapes.push_back(New); }

    // Create a list of entries from a block
    void GetBlocksOut(Block *Source, BlockVec& Entries) {
      for (int i = 0; i < Source->BranchesOut.size(); i++) {
        Entries.push_back(Source->BranchesOut[i]->Target);
      }
    }

    Shape *MakeLoop(BlockSet &Blocks, BlockVec& Entries) {
      // Find the inner blocks in this loop. Proceed backwards from the entries until
      // you reach a seen block, collecting as you go.
      BlockSet InnerBlocks;
      BlockVec Queue = Entries;
      while (Queue.size() > 0) {
        Block *Curr = Queue.back();
        Queue.pop_back();
        if (InnerBlocks.find(Curr) != InnerBlocks.end()) {
          // This element is new, mark it as inner and remove from outer
          InnerBlocks.insert(Curr);
          Blocks.erase(Curr);
          // Add the elements prior to it
          for (int i = 0; i < Curr->BranchesIn.size(); i++) {
            Queue.push_back(Curr.BranchesIn[i]->Target);
          }
        }
      }
      BlockVec NextEntries;
      BlockSet NextEntriesSet;
      for (BlockSet::iterator iter = InnerBlocks.begin(); iter != InnerBlocks.end(); iter++) {
        Block *Curr = *iter;
        for (int i = 0; i < Curr->BranchesOut.size(); i++) {
          Block *Possible = Curr->BranchesOut[i]->Target;
          if (InnerBlocks.find(Possible) == InnerBlocks().end() &&
              NextEntriesSet.find(Possible) == NextEntriesSet.find(Possible)) {
            NextEntries.push_back(Possible);
            NextEntriesSet.insert(Possible);
          }
        }
      }
      // TODO: Hoist additional blocks into the loop
      // Solipsize the loop, replacing with break/continue and marking branches as Processed (will not affect later calculations)
      // A. Branches to the loop entries become a continue to this shape
      ...
      // B. Branches to outside the loop (a next entry) become breaks on this shape
      ...
      // Finish up
      Shape *Inner = MakeBlock(InnerBlocks, Entries);
      Shape *Loop = Notice(new LoopShape(Inner));
      Loop->Next = NextEntries.size() > 0 ? MakeBlock(Blocks, NextEntries) : NULL;
      return Loop;
    }

    void FindIndependentGroups(BlockSet &Blocks, BlockVec &Entries, BlockBlockVec& IndependentGroups) {
    }

    Shape *MakeMultiple(BlockSet &Blocks, BlockVec &Entries, BlockBlockVec& IndependentGroups) {
      return NULL;
    }

    // Main function.
    // Process a set of blocks with specified entries, returns a shape
    Shape *Process(BlockSet &Blocks, BlockVec& Entries) {
      if (Entries.size() == 1) {
        Block *Curr = Entries[0];
        if (Curr->BranchesIn.size() == 0) {
          // One entry, no looping ==> Simple
          Shape *Ret = Notice(new SimpleShape(Curr));
          if (Blocks.size() > 1) {
            Blocks.erase(Curr);
            Entries.clear();
            GetEntries(Curr, Entries);
            Ret->Next = Process(Blocks, Entries);
          }
          return Ret;
        }
        // One entry, looping ==> Loop
        return MakeLoop(Blocks, Entries);
      }
      // More than one entry, try to eliminate through a Multiple groups of
      // independent blocks from an entry/ies
      BlockBlockVec IndependentGroups;
      FindIndependentGroups(Blocks, Entries, IndependentGroups);
      if (IndependentGroups.size() > 0) {
        // Independent groups removable ==> Multiple
        return MakeMultiple(Blocks, Entries, IndependentGroups);
      }
      // No independent groups, must be loopable ==> Loop
      return MakeLoop(Blocks, Entries);
    }
  }

  BlockSet AllBlocks;
  for (int i = 0; i < Blocks.size(); i++) {
    AllBlocks.insert(Blocks[i]);
  }
  Root = Recursor(this)::Process(AllBlocks);
}

