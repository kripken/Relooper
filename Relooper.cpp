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
      Curr->BranchesOut[j]->Target->BranchesIn.push_back(new Branch(Curr)); // leaky?
    }
  }

  // ...

}

