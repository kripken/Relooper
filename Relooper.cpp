
#include "Relooper.h"

#include <string.h>
#include <stdlib.h>
#include <list>

// TODO: move all set to unorderedset

static void PrintDebug(const char *Format, ...);

struct Indenter {
  static int CurrIndent;

  static void Indent() { CurrIndent++; }
  static void Unindent() { CurrIndent--; }
};

static void PrintIndented(const char *Format, ...);
static void PutIndented(const char *String);

static char *__OutputBuffer__ = NULL;

void PrintIndented(const char *Format, ...) {
  assert(__OutputBuffer__);
  for (int i = 0; i < Indenter::CurrIndent*2; i++, __OutputBuffer__++) *__OutputBuffer__ = ' ';
  va_list Args;
  va_start(Args, Format);
  __OutputBuffer__ += vsprintf(__OutputBuffer__, Format, Args);
  va_end(Args);
}

void PutIndented(const char *String) {
  assert(__OutputBuffer__);
  for (int i = 0; i < Indenter::CurrIndent*2; i++, __OutputBuffer__++) *__OutputBuffer__ = ' ';
  strcpy(__OutputBuffer__, String);
  __OutputBuffer__ += strlen(String);
  *__OutputBuffer__++ = '\n';
  *__OutputBuffer__ = 0;
}

// Indenter

int Indenter::CurrIndent = 0;

// Branch

Branch::Branch(const char *ConditionInit) : Ancestor(NULL), Set(true) {
  Condition = ConditionInit ? strdup(ConditionInit) : NULL;
}

Branch::~Branch() {
  if (Condition) free((void*)Condition);
}

void Branch::Render(Block *Target) {
  if (Set) PrintIndented("label = %d;\n", Target->Id);
  if (Ancestor) {
    if (Type == Direct) {
      PrintIndented("/* direct */\n");
    } else {
      PrintIndented("%s L%d;\n", Type == Break ? "break" : "continue", Ancestor->Id);
    }
  }
}

// Block

int Block::IdCounter = 0;

Block::Block(const char *CodeInit) : Parent(NULL), Reachable(false), Id(Block::IdCounter++), DefaultTarget(NULL) {
  Code = strdup(CodeInit);
}

Block::~Block() {
  if (Code) free((void*)Code);
  for (BlockBranchMap::iterator iter = ProcessedBranchesIn.begin(); iter != ProcessedBranchesIn.end(); iter++) {
    delete iter->second;
  }
  for (BlockBranchMap::iterator iter = ProcessedBranchesOut.begin(); iter != ProcessedBranchesOut.end(); iter++) {
    delete iter->second;
  }
  // XXX If not reachable, expected to have branches here. But need to clean them up to prevent leaks!
  assert(!Reachable || BranchesIn.size() == 0);
  assert(!Reachable || BranchesOut.size() == 0);
}

void Block::AddBranchTo(Block *Target, const char *Condition) {
  BranchesOut[Target] = new Branch(Condition);
}

void Block::Render() {
  if (Code) {
    // Print code in an indented manner, even over multiple lines
    char *Start = const_cast<char*>(Code);
    while (*Start) {
      char *End = strchr(Start, '\n');
      if (End) *End = 0;
      PutIndented(Start);
      if (End) *End = '\n'; else break;
      Start = End+1;
    }
  }

  if (!ProcessedBranchesOut.size()) return;

  bool SetLabel = true; // in some cases it is clear we can avoid setting label, see later

  if (ProcessedBranchesOut.size() == 1 && ProcessedBranchesOut.begin()->second->Type == Branch::Direct) {
    SetLabel = false;
  }

  // Fusing: If the next is a Multiple, we can fuse it with this block. Note
  // that we must be the Inner of a Simple, so fusing means joining a Simple
  // to a Multiple. What happens there is that all options in the Multiple
  // *must* appear in the Simple (the Simple is the only one reaching the
  // Multiple), so we can remove the Multiple and add its independent groups
  // into the Simple's branches.
  MultipleShape *Fused = dynamic_cast<MultipleShape*>(Parent->Next);
  if (Fused) {
    PrintDebug("Fusing Multiple to Simple\n");
    Parent->Next = Parent->Next->Next;
    Fused->RenderLoopPrefix();

    // When the Multiple has the same number of groups as we have branches,
    // they will all be fused, so it is safe to not set the label at all
    if (SetLabel && Fused->InnerMap.size() == ProcessedBranchesOut.size()) {
      SetLabel = false;
    }
  }

  if (!DefaultTarget) { // If no default specified, it is the last
    for (BlockBranchMap::iterator iter = ProcessedBranchesOut.begin(); iter != ProcessedBranchesOut.end(); iter++) {
      if (!iter->second->Condition) {
        assert(!DefaultTarget); // Must be exactly one default
        DefaultTarget = iter->first;
      }
    }
  }
  assert(DefaultTarget); // Must be a default

  bool First = true;
  for (BlockBranchMap::iterator iter = ProcessedBranchesOut.begin(); iter != ProcessedBranchesOut.end(); iter++) {
    Block *Target = iter->first;
    Branch *Details = iter->second;
    if (Target == DefaultTarget) continue; // done at the end
    assert(Details->Condition); // must have a condition if this is not the default target
    PrintIndented("%sif (%s) {\n", First ? "" : "} else ", Details->Condition);
    First = false;
    Indenter::Indent();
    if (!SetLabel) Details->Set = false;
    Details->Render(Target);
    if (Fused && Fused->InnerMap.find(Target) != Fused->InnerMap.end()) {
      Fused->InnerMap.find(Target)->second->Render();
    }
    Indenter::Unindent();
  }
  if (DefaultTarget) {
    if (!First) {
      PrintIndented("} else {\n");
      Indenter::Indent();
    }
    Branch *Details = ProcessedBranchesOut[DefaultTarget];
    if (!SetLabel) Details->Set = false;
    Details->Render(DefaultTarget);
    if (Fused && Fused->InnerMap.find(DefaultTarget) != Fused->InnerMap.end()) {
      Fused->InnerMap.find(DefaultTarget)->second->Render();
    }
    if (!First) {
      Indenter::Unindent();
    }
  }
  if (!First) PrintIndented("}\n");

  if (Fused) {
    Fused->RenderLoopPostfix();
  }
}

// Shape

int Shape::IdCounter = 0;

// MultipleShape

void MultipleShape::RenderLoopPrefix() {
  if (NeedLoop) {
    PrintIndented("L%d: do {\n", Id);
    Indenter::Indent();
  }
}

void MultipleShape::RenderLoopPostfix() {
  if (NeedLoop) {
    Indenter::Unindent();
    PrintIndented("} while(0);\n");
  }
}

void MultipleShape::Render() {
  RenderLoopPrefix();
  bool First = true;
  for (BlockShapeMap::iterator iter = InnerMap.begin(); iter != InnerMap.end(); iter++) {
    PrintIndented("%sif (label == %d) {\n", First ? "" : "else ", iter->first->Id);
    First = false;
    Indenter::Indent();
    iter->second->Render();
    Indenter::Unindent();
    PrintIndented("}\n");
  }
  RenderLoopPostfix();
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

Relooper::Relooper() : Root(NULL) {
}

Relooper::~Relooper() {
  for (int i = 0; i < Blocks.size(); i++) delete Blocks[i];
  for (int i = 0; i < Shapes.size(); i++) delete Shapes[i];
}

void Relooper::AddBlock(Block *New) {
  Blocks.push_back(New);
}

void Relooper::Calculate(Block *Entry) {
  Shapes.reserve(Blocks.size()); // vague heuristic, better than nothing

  // Add incoming branches
  for (int i = 0; i < Blocks.size(); i++) {
    Block *Curr = Blocks[i];
    for (BlockBranchMap::iterator iter = Curr->BranchesOut.begin(); iter != Curr->BranchesOut.end(); iter++) {
      iter->first->BranchesIn[Curr] = new Branch(iter->second->Condition);
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

    // Create a list of entries from a block. If LimitTo is provided, only results in that set
    // will appear
    void GetBlocksOut(Block *Source, BlockSet& Entries, BlockSet *LimitTo=NULL) {
      for (BlockBranchMap::iterator iter = Source->BranchesOut.begin(); iter != Source->BranchesOut.end(); iter++) {
        if (!LimitTo || LimitTo->find(iter->first) != LimitTo->end()) {
          Entries.insert(iter->first);
        }
      }
    }

    // Converts/processes all branchings to a specific target
    void Solipsize(Block *Target, Branch::FlowType Type, Shape *Ancestor, BlockSet &From) {
      PrintDebug("Solipsizing branches into %d\n", Target->Id);
      Debugging::Dump(From, "  relevant to solipsize: ");
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
        if (MultipleShape *Multiple = dynamic_cast<MultipleShape*>(Ancestor)) {
          Multiple->NeedLoop = true; // We are breaking out of this Multiple, so need a loop
        }
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
      Notice(Simple);
      Simple->Inner = Inner;
      Inner->Parent = Simple;
      Inner->Reachable = true;
      if (Blocks.size() > 1) {
        Blocks.erase(Inner);
        BlockSet Entries;
        GetBlocksOut(Inner, Entries, &Blocks);
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
      typedef std::map<Block*, Block*> BlockBlockMap;
      typedef std::list<Block*> BlockList;

      struct HelperClass {
        BlockBlockSetMap& IndependentGroups;
        BlockBlockMap Ownership; // For each block, which entry it belongs to. We have reached it from there.

        HelperClass(BlockBlockSetMap& IndependentGroupsInit) : IndependentGroups(IndependentGroupsInit) {}
        void InvalidateWithChildren(Block *New) { // TODO: rename New
          BlockList ToInvalidate; // Being in the list means you need to be invalidated
          ToInvalidate.push_back(New);
          while (ToInvalidate.size() > 0) {
            Block *Invalidatee = ToInvalidate.front();
            ToInvalidate.pop_front();
            Block *Owner = Ownership[Invalidatee];
            if (IndependentGroups.find(Owner) != IndependentGroups.end()) { // Owner may have been invalidated, do not add to IndependentGroups!
              IndependentGroups[Owner].erase(Invalidatee);
            }
            if (Ownership[Invalidatee]) { // may have been seen before and invalidated already
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
        }
      };
      HelperClass Helper(IndependentGroups);

      // We flow out from each of the entries, simultaneously.
      // When we reach a new block, we add it as belonging to the one we got to it from.
      // If we reach a new block that is already marked as belonging to someone, it is reachable by
      // two entries and is not valid for any of them. Remove it and all it can reach that have been
      // visited.

      BlockList Queue; // Being in the queue means we just added this item, and we need to add its children
      for (BlockSet::iterator iter = Entries.begin(); iter != Entries.end(); iter++) {
        Block *Entry = *iter;
        Helper.Ownership[Entry] = Entry;
        IndependentGroups[Entry].insert(Entry);
        Queue.push_back(Entry);
      }
      while (Queue.size() > 0) {
        Block *Curr = Queue.front();
        Queue.pop_front();
        Block *Owner = Helper.Ownership[Curr]; // Curr must be in the ownership map if we are in the queue
        if (!Owner) continue; // we have been invalidated meanwhile after being reached from two entries
        // Add all children
        for (BlockBranchMap::iterator iter = Curr->BranchesOut.begin(); iter != Curr->BranchesOut.end(); iter++) {
          Block *New = iter->first;
          BlockBlockMap::iterator Known = Helper.Ownership.find(New);
          if (Known == Helper.Ownership.end()) {
            // New node. Add it, and put it in the queue
            Helper.Ownership[New] = Owner;
            IndependentGroups[Owner].insert(New);
            Queue.push_back(New);
            continue;
          }
          Block *NewOwner = Known->second;
          if (!NewOwner) continue; // We reached an invalidated node
          if (NewOwner != Owner) {
            // Invalidate this and all reachable that we have seen - we reached this from two locations
            Helper.InvalidateWithChildren(New);
          }
          // otherwise, we have the same owner, so do nothing
        }
      }

      // Having processed all the interesting blocks, we remain with just one potential issue:
      // If a->b, and a was invalidated, but then b was later reached by someone else, we must
      // invalidate b. To check for this, we go over all elements in the independent groups,
      // if an element has a parent which does *not* have the same owner, we must remove it
      // and all its children.

      for (BlockSet::iterator iter = Entries.begin(); iter != Entries.end(); iter++) {
        BlockSet &CurrGroup = IndependentGroups[*iter];
        BlockList ToInvalidate;
        for (BlockSet::iterator iter = CurrGroup.begin(); iter != CurrGroup.end(); iter++) {
          Block *Child = *iter;
          for (BlockBranchMap::iterator iter = Child->BranchesIn.begin(); iter != Child->BranchesIn.end(); iter++) {
            Block *Parent = iter->first;
            if (Helper.Ownership[Parent] != Helper.Ownership[Child]) {
              ToInvalidate.push_back(Child);
            }
          }
        }
        while (ToInvalidate.size() > 0) {
          Block *Invalidatee = ToInvalidate.front();
          ToInvalidate.pop_front();
          Helper.InvalidateWithChildren(Invalidatee);
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

    Shape *MakeMultiple(BlockSet &Blocks, BlockSet& Entries, BlockBlockSetMap& IndependentGroups) {
      PrintDebug("creating multiple block with %d inner groups\n", IndependentGroups.size());
      MultipleShape *Multiple = new MultipleShape();
      Notice(Multiple);
      BlockSet NextEntries, CurrEntries;
      for (BlockBlockSetMap::iterator iter = IndependentGroups.begin(); iter != IndependentGroups.end(); iter++) {
        Block *CurrEntry = iter->first;
        BlockSet &CurrBlocks = iter->second;
        PrintDebug("  multiple group with entry %d:\n", CurrEntry->Id);
        Debugging::Dump(CurrBlocks, "    ");
        // Create inner block
        CurrEntries.clear();
        CurrEntries.insert(CurrEntry);
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
              Solipsize(CurrTarget, Branch::Break, Multiple, CurrBlocks); 
            }
            iter = Next; // increment carefully because Solipsize can remove us
          }
        }
        Multiple->InnerMap[CurrEntry] = Process(CurrBlocks, CurrEntries);
      }
      Debugging::Dump(Blocks, "  remaining blocks after multiple:");
      // Add entries not handled as next entries, they are deferred
      for (BlockSet::iterator iter = Entries.begin(); iter != Entries.end(); iter++) {
        Block *Entry = *iter;
        if (IndependentGroups.find(Entry) == IndependentGroups.end()) {
          NextEntries.insert(Entry);
        }
      }
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
          return MakeMultiple(Blocks, Entries, IndependentGroups);
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
      PrintDebug("Adding block %d (%s)\n", Blocks[i]->Id, Blocks[i]->Code);
      for (BlockBranchMap::iterator iter = Blocks[i]->BranchesOut.begin(); iter != Blocks[i]->BranchesOut.end(); iter++) {
        PrintDebug("  with branch out to %d\n", iter->first->Id);
      }
    }
  }
  BlockSet Entries;

  Entries.insert(Entry);
  Root = Recursor(this).Process(AllBlocks, Entries);
}

void Relooper::SetOutputBuffer(char *Buffer) {
  __OutputBuffer__ = Buffer;
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

static void PrintDebug(const char *Format, ...) {
  if (Debugging::On) {
    printf("// ");
    va_list Args;
    va_start(Args, Format);
    vprintf(Format, Args);
    va_end(Args);
  }
}


// C API - useful for binding to other languages

typedef std::map<void*, int> VoidIntMap;
VoidIntMap __blockDebugMap__; // maps block pointers in currently running code to block ids, for generated debug output

extern "C" {

void rl_set_output_buffer(char *buffer) {
  if (Debugging::On) {
    printf("#include \"Relooper.h\"\n");
    printf("int main() {\n");
    printf("  char buffer[100000];\n");
    printf("  rl_set_output_buffer(buffer);\n");
  }
  Relooper::SetOutputBuffer(buffer);
}

void *rl_new_block(const char *text) {
  Block *ret = new Block(text);
  if (Debugging::On) {
    printf("  void *b%d = rl_new_block(\"// code %d\");\n", ret->Id, ret->Id);
    __blockDebugMap__[ret] = ret->Id;
    printf("  block_map[%d] = b%d;\n", ret->Id, ret->Id);
  }
  return ret;
}

void rl_delete_block(void *block) {
  if (Debugging::On) {
    printf("  rl_delete_block(block_map[%d]);\n", ((Block*)block)->Id);
  }
  delete (Block*)block;
}

void rl_block_add_branch_to(void *from, void *to, const char *condition) {
  if (Debugging::On) {
    printf("  rl_block_add_branch_to(block_map[%d], block_map[%d], %s%s%s);\n", ((Block*)from)->Id, ((Block*)to)->Id, condition ? "\"" : "", condition ? condition : "NULL", condition ? "\"" : "");
  }
  ((Block*)from)->AddBranchTo((Block*)to, condition);
}

void *rl_new_relooper() {
  if (Debugging::On) {
    printf("  void *block_map[10000];\n");
    printf("  void *rl = rl_new_relooper();\n");
  }
  return new Relooper;
}

void rl_delete_relooper(void *relooper) {
  delete (Relooper*)relooper;
}

void rl_relooper_add_block(void *relooper, void *block) {
  if (Debugging::On) {
    printf("  rl_relooper_add_block(rl, block_map[%d]);\n", ((Block*)block)->Id);
  }
  ((Relooper*)relooper)->AddBlock((Block*)block);
}

void rl_relooper_calculate(void *relooper, void *entry) {
  if (Debugging::On) {
    printf("  rl_relooper_calculate(rl, block_map[%d]);\n", ((Block*)entry)->Id);
    printf("  rl_relooper_render(rl);\n");
    printf("  rl_delete_relooper(rl);\n");
    printf("  puts(buffer);\n");
    printf("  return 0;\n");
    printf("}\n");
  }
  ((Relooper*)relooper)->Calculate((Block*)entry);
}

void rl_relooper_render(void *relooper) {
  ((Relooper*)relooper)->Render();
}

void rl_set_debugging(int on) {
  Debugging::On = on;
}

}

