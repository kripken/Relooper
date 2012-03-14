
#include "Relooper.h"

#include <list>

// TODO: move all set to unorderedset

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

void Branch::Render(Block *Target) {
  if (Set) PrintIndented("label = %d;\n", Target->Id);
  if (Ancestor) {
    if (Type == Direct) {
      PrintIndented("[direct]\n");
    } else {
      PrintIndented("%s L%d;\n", Type == Break ? "break" : "continue", Ancestor->Id);
    }
  }
}

// Block

int Block::IdCounter = 0;

// Shape

int Shape::IdCounter = 0;

// MultipleShape

void MultipleShape::Render() {
  bool NeedLoop = true; // TODO
  if (NeedLoop) {
    PrintIndented("L%d: do {\n", Id);
    Indenter::Indent();
  }
  bool First = true;
  for (BlockShapeMap::iterator iter = InnerMap.begin(); iter != InnerMap.end(); iter++) {
    PrintIndented("%sif (label == %d) {\n", First ? "" : "else ", iter->first->Id);
    First = false;
    Indenter::Indent();
    iter->second->Render();
    Indenter::Unindent();
    PrintIndented("}\n");
  }
  if (NeedLoop) {
    Indenter::Unindent();
    PrintIndented("} while(0);\n");
  }
  if (Next) Next->Render();
};

// LoopShape

void LoopShape::Render() {
  PrintIndented("L%d: while(1) {\n", Id);
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

void Relooper::Calculate(Block *Entry) {
  Shapes.reserve(Blocks.size()/2); // vague heuristic, better than nothing

  // Add incoming branches
  for (int i = 0; i < Blocks.size(); i++) {
    Block *Curr = Blocks[i];
    for (BlockBranchMap::iterator iter = Curr->BranchesOut.begin(); iter != Curr->BranchesOut.end(); iter++) {
      iter->first->BranchesIn[Curr] = new Branch(); // XXX leaky
    }
  }

  // Recursively process the graph

  struct Recursor {
    Relooper *Parent;
    Recursor(Relooper *ParentInit) : Parent(ParentInit) {}

    // Add a shape to the list of shapes in this Relooper calculation
    void Notice(Shape *New) {
      Parent->Shapes.push_back(New);
    }

    // Create a list of entries from a block
    void GetBlocksOut(Block *Source, BlockSet& Entries) {
      for (BlockBranchMap::iterator iter = Source->BranchesOut.begin(); iter != Source->BranchesOut.end(); iter++) {
        Entries.insert(iter->first);
      }
    }

    // Converts/processes all branchings to a specific target
    void Solipsize(Block *Target, Branch::FlowType Type, Shape *Ancestor, BlockSet &From) {
      PrintDebug("Solipsizing branches into %d\n", Target->Id);
      for (BlockBranchMap::iterator iter = Target->BranchesIn.begin(); iter != Target->BranchesIn.end();) {
        Block *Prior = iter->first;
        if (From.find(Prior) == From.end()) {
          iter++;
          continue;
        }
        Branch *TargetIn = iter->second;
        Branch *PriorOut = Prior->BranchesOut[Target];
        PriorOut->Ancestor = Ancestor; // Do we need this info
        PriorOut->Type = Type;         // on TargetIn too?
        iter++; // carefully increment iter before erasing
        Target->BranchesIn.erase(Prior);
        Target->ProcessedBranchesIn[Prior] = TargetIn;
        Prior->BranchesOut.erase(Target);
        Prior->ProcessedBranchesOut[Target] = PriorOut;
        PrintDebug("  eliminated branch from %d\n", Prior->Id);
      }
    }

    Shape *MakeSimple(BlockSet &Blocks, Block *Inner) {
      PrintDebug("creating simple block with block #%d\n", Inner->Id);
      SimpleShape *Simple = new SimpleShape;
      Simple->Inner = Inner;
      Inner->Parent = Simple;
      Notice(Simple);
      if (Blocks.size() > 1) {
        Blocks.erase(Inner);
        BlockSet Entries;
        GetBlocksOut(Inner, Entries);
        BlockSet JustInner;
        JustInner.insert(Inner);
        for (BlockSet::iterator iter = Entries.begin(); iter != Entries.end(); iter++) {
          Solipsize(*iter, Branch::Direct, Simple, JustInner);
        }
        Simple->Next = Process(Blocks, Entries);
      }
      return Simple;
    }

    Shape *MakeLoop(BlockSet &Blocks, BlockSet& Entries) {
      // Find the inner blocks in this loop. Proceed backwards from the entries until
      // you reach a seen block, collecting as you go.
      BlockSet InnerBlocks;
      BlockSet Queue = Entries;
      while (Queue.size() > 0) {
        Block *Curr = *(Queue.begin());
        Queue.erase(Queue.begin());
        if (InnerBlocks.find(Curr) == InnerBlocks.end()) {
          // This element is new, mark it as inner and remove from outer
          InnerBlocks.insert(Curr);
          Blocks.erase(Curr);
          // Add the elements prior to it
          for (BlockBranchMap::iterator iter = Curr->BranchesIn.begin(); iter != Curr->BranchesIn.end(); iter++) {
            Queue.insert(iter->first);
          }
        }
      }
      assert(InnerBlocks.size() > 0);

      BlockSet NextEntries;
      for (BlockSet::iterator iter = InnerBlocks.begin(); iter != InnerBlocks.end(); iter++) {
        Block *Curr = *iter;
        for (BlockBranchMap::iterator iter = Curr->BranchesOut.begin(); iter != Curr->BranchesOut.end(); iter++) {
          Block *Possible = iter->first;
          if (InnerBlocks.find(Possible) == InnerBlocks.end() &&
              NextEntries.find(Possible) == NextEntries.find(Possible)) {
            NextEntries.insert(Possible);
          }
        }
      }

      PrintDebug("creating loop block:\n");
      Debugging::Dump(InnerBlocks, "  inner blocks:");
      Debugging::Dump(Entries, "  inner entries:");
      Debugging::Dump(Blocks, "  outer blocks:");
      Debugging::Dump(NextEntries, "  outer entries:");

      // TODO: Optionally hoist additional blocks into the loop

      LoopShape *Loop = new LoopShape();
      Notice(Loop);

      // Solipsize the loop, replacing with break/continue and marking branches as Processed (will not affect later calculations)
      // A. Branches to the loop entries become a continue to this shape
      for (BlockSet::iterator iter = Entries.begin(); iter != Entries.end(); iter++) {
        Solipsize(*iter, Branch::Continue, Loop, InnerBlocks);
      }
      // B. Branches to outside the loop (a next entry) become breaks on this shape
      for (BlockSet::iterator iter = NextEntries.begin(); iter != NextEntries.end(); iter++) {
        Solipsize(*iter, Branch::Break, Loop, InnerBlocks);
      }
      // Finish up
      Shape *Inner = Process(InnerBlocks, Entries);
      Loop->Inner = Inner;
      Loop->Next = Process(Blocks, NextEntries);
      return Loop;
    }

    // For each entry, find the independent group reachable by it. The independent group is
    // the entry itself, plus all the blocks it can reach that cannot be directly reached by another entry. Note that we
    // ignore directly reaching the entry itself by another entry.
    void FindIndependentGroups(BlockSet &Blocks, BlockSet &Entries, BlockBlockSetMap& IndependentGroups) {
      // We flow out from each of the entries, simultaneously.
      // When we reach a new block, we add it as belonging to the one we got to it from.
      // If we reach a new block that is already marked as belonging to someone, it is reachable by
      // two entries and is not valid for any of them. Remove it and all it can reach that have been
      // visited.
      typedef std::map<Block*, Block*> BlockBlockMap;
      typedef std::list<Block*> BlockList;

      BlockBlockMap Ownership; // For each block, Which entry it belongs to. We have reached it from there.
      BlockList Queue; // Being in the queue means we just added this item, and we need to add its children
      for (BlockSet::iterator iter = Entries.begin(); iter != Entries.end(); iter++) {
        Block *Entry = *iter;
        Ownership[Entry] = Entry;
        IndependentGroups[Entry].insert(Entry);
        Queue.push_back(Entry);
      }
      while (Queue.size() > 0) {
        Block *Curr = Queue.front();
        Queue.pop_front();
        Block *Owner = Ownership[Curr]; // Curr must be in the ownership map if we are in the queue
        if (!Owner) continue; // we have been invalidated meanwhile after being reached from two entries
        // Add all children
        for (BlockBranchMap::iterator iter = Curr->BranchesOut.begin(); iter != Curr->BranchesOut.end(); iter++) {
          Block *New = iter->first;
          BlockBlockMap::iterator Known = Ownership.find(New);
          if (Known == Ownership.end()) {
            // New node. Add it, and put it in the queue
            Ownership[New] = Owner;
            IndependentGroups[Owner].insert(New);
            Queue.push_back(New);
            continue;
          }
          Block *NewOwner = Known->second;
          if (!NewOwner) continue; // We reached an invalidated node
          if (NewOwner != Owner) {
            // Invalidate this and all reachable that we have seen - we reached this from two locations
            BlockList ToInvalidate; // Being in the list means you need to be invalidated
            ToInvalidate.push_back(New);
            while (ToInvalidate.size() > 0) {
              Block *Invalidatee = ToInvalidate.front();
              ToInvalidate.pop_front();
              IndependentGroups[Ownership[Invalidatee]].erase(Invalidatee);
              Ownership[Invalidatee] = NULL;
              for (BlockBranchMap::iterator iter = Invalidatee->BranchesOut.begin(); iter != Invalidatee->BranchesOut.end(); iter++) {
                Block *Target = iter->first;
                BlockBlockMap::iterator Known = Ownership.find(Target);
                if (Known != Ownership.end()) {
                  Block *TargetOwner = Known->second;
                  if (TargetOwner) {
                    ToInvalidate.push_back(Target);
                  }
                }
              }
            }
          }
          // otherwise, we have the same owner, so do nothing
        }
      }

      // Remove empty groups
      for (BlockSet::iterator iter = Entries.begin(); iter != Entries.end(); iter++) {
        if (IndependentGroups[*iter].size() == 0) {
          IndependentGroups.erase(*iter);
        }
      }

      if (Debugging::On) {
        PrintDebug("Investigated independent groups:\n");
        for (BlockBlockSetMap::iterator iter = IndependentGroups.begin(); iter != IndependentGroups.end(); iter++) {
          Debugging::Dump(iter->second, " group: ");
        }
      }
    }

    Shape *MakeMultiple(BlockSet &Blocks, BlockBlockSetMap& IndependentGroups) {
      PrintDebug("creating multiple block with %d inner groups\n", IndependentGroups.size());
      MultipleShape *Multiple = new MultipleShape();
      BlockSet NextEntries, CurrEntries;
      for (BlockBlockSetMap::iterator iter = IndependentGroups.begin(); iter != IndependentGroups.end(); iter++) {
        Block *CurrEntry = iter->first;
        BlockSet &CurrBlocks = iter->second;
        PrintDebug("  group with entry %d:\n", CurrEntry->Id);
        Debugging::Dump(CurrBlocks, "    ");
        // Create inner block
        CurrEntries.clear();
        CurrEntries.insert(CurrEntry);
        Multiple->InnerMap[CurrEntry] = Process(iter->second, CurrEntries);
        for (BlockSet::iterator iter = CurrBlocks.begin(); iter != CurrBlocks.end(); iter++) {
          Block *CurrInner = *iter;
          // Remove the block from the remaining blocks
          Blocks.erase(CurrInner);
          // Find new next entries and fix branches to them
          for (BlockBranchMap::iterator iter = CurrInner->BranchesOut.begin(); iter != CurrInner->BranchesOut.end();) {
            Block *CurrTarget = iter->first;
            BlockBranchMap::iterator Next = iter;
            Next++;
            if (CurrBlocks.find(CurrTarget) == CurrBlocks.end()) {
              NextEntries.insert(CurrTarget);
              Solipsize(CurrTarget, Branch::Break, Multiple, Blocks); 
            }
            iter = Next; // increment carefully because Solipsize can remove us
          }
        }
      }
      Debugging::Dump(Blocks, "  remaining blocks:");
      Multiple->Next = Process(Blocks, NextEntries);
      return Multiple;
    }

    // Main function.
    // Process a set of blocks with specified entries, returns a shape
    Shape *Process(BlockSet &Blocks, BlockSet& Entries) {
      PrintDebug("Process() called\n");
      Debugging::Dump(Blocks, "  blocks : ");
      Debugging::Dump(Entries, "  entries: ");

      if (Entries.size() == 0) return NULL;
      if (Entries.size() == 1) {
        Block *Curr = *(Entries.begin());
        if (Curr->BranchesIn.size() == 0) {
          // One entry, no looping ==> Simple
          return MakeSimple(Blocks, Curr);
        }
        // One entry, looping ==> Loop
        return MakeLoop(Blocks, Entries);
      }
      // More than one entry, try to eliminate through a Multiple groups of
      // independent blocks from an entry/ies. It is important to remove through
      // multiples as opposed to looping since the former is more performant.
      BlockBlockSetMap IndependentGroups;
      FindIndependentGroups(Blocks, Entries, IndependentGroups);

      PrintDebug("Independent groups: %d\n", IndependentGroups.size());

      if (IndependentGroups.size() > 0) {
        // We can handle a group in a multiple if its entry cannot be reached by another group.
        // Note that it might be reachable by itself - a loop. But that is fine, we will create
        // a loop inside the multiple block (which is the performant order to do it).
        for (BlockBlockSetMap::iterator iter = IndependentGroups.begin(); iter != IndependentGroups.end();) {
          Block *Entry = iter->first;
          BlockSet &Group = iter->second;
          BlockBlockSetMap::iterator curr = iter++; // iterate carefully, we may delete
          for (BlockBranchMap::iterator iterBranch = Entry->BranchesIn.begin(); iterBranch != Entry->BranchesIn.end(); iterBranch++) {
            Block *Origin = iterBranch->first;
            if (Group.find(Origin) == Group.end()) {
              // Reached from outside the group, so we cannot handle this
              PrintDebug("Cannot handle group with entry %d because of incoming branch from %d\n", Entry->Id, Origin->Id);
              IndependentGroups.erase(curr);
              break;
            }
          }
        }

        PrintDebug("Handleable independent groups: %d\n", IndependentGroups.size());

        if (IndependentGroups.size() > 0) {
          // Some groups removable ==> Multiple
          return MakeMultiple(Blocks, IndependentGroups);
        }
      }
      // No independent groups, must be loopable ==> Loop
      return MakeLoop(Blocks, Entries);
    }
  };

  BlockSet AllBlocks;
  for (int i = 0; i < Blocks.size(); i++) {
    AllBlocks.insert(Blocks[i]);
    if (Debugging::On) {
      PrintDebug("Adding block %d\n", Blocks[i]->Id);
      for (BlockBranchMap::iterator iter = Blocks[i]->BranchesOut.begin(); iter != Blocks[i]->BranchesOut.end(); iter++) {
        PrintDebug("  with branch out to %d\n", iter->first->Id);
      }
    }
  }
  BlockSet Entries;
  Entries.insert(Entry);
  Root = Recursor(this).Process(AllBlocks, Entries);
}

// Debugging

bool Debugging::On = false; // TODO: make this a compile-time #define

void Debugging::Dump(BlockSet &Blocks, const char *prefix) {
  if (Debugging::On) {
    if (prefix) printf("%s ", prefix);
    for (BlockSet::iterator iter = Blocks.begin(); iter != Blocks.end(); iter++) {
      printf("%d ", (*iter)->Id);
    }
    printf("\n");
  }
}

void PrintDebug(const char *Format, ...) {
  if (Debugging::On) {
    va_list Args;
    va_start(Args, Format);
    vprintf(Format, Args);
    va_end(Args);
  }
}

