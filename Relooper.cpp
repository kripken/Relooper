#include "Relooper.h"

// Renderer

static int Renderer::CurrIndent = 0;

// Branch

struct Branch {
  Block *Target; // The block we branch to
  Shape *Ancestor; // If not NULL, this shape is the relevant one for purposes of getting to the target block. We break or continue on it
  bool Break; // If Ancestor is not NULL, this says whether to break or continue
  bool Set; // Set the label variable

  Branch(Block *BlockInit) Block(BlockInit), Ancestor(NULL), Set(false) {}

  // Prints out the branch
  void Render();
};

void Branch::Render() {
  if (Set) Renderer::Print("label = %d;\n", Target->Id);
  if (Ancestor) {
    Renderer::Print("%s L%d;\n", Break ? "break" : "continue", Ancestor->Id);
  }
}

// Block

int Block::IdCounter = 0;

// Shape

int Shape::IdCounter = 0;

// MultipleShape

void MultipleShape::Render() {
  bool First = true;
  for (BlockIdShapeMap::iterator iter = InnerMap.begin(); iter != InnerMap.end(); iter++) {
    Renderer::Print("%s if (label == %d) {\n", First ? "" : "else ", iter->first);
    First = false;
    Renderer::Indent();
    iter->second->Render();
    Renderer::Unindent();
    Relooper::Print("}\n");
  }
  if (Next) Next->Render();
};

// LoopShape

void LoopShape::Render() {
  Renderer::Print("while(1) {\n");
  Renderer::Indent();
  Inner->Render();
  Renderer::Unindent();
  Relooper::Print("}\n");
  if (Next) Next->Render();
};

// EmulatedShape

void MultipleShape::Render() {
  Renderer::Print("while(1) {\n");
  Renderer::Indent();
  Renderer::Print("switch(label) {\n");
  Renderer::Indent();
  for (int i = 0; i < Blocks.size(); i++) {
    Block &Curr = Blocks[i];
    Renderer::Print("case %d: {\n", Curr->Id);
    Renderer::Indent();
    Curr->Render();
    Renderer::Print("break;\n");
    Renderer::Unindent();
    Relooper::Print("}\n");
  }
  Renderer::Unindent();
  Relooper::Print("}\n");
  Renderer::Unindent();
  Relooper::Print("}\n");
  if (Next) Next->Render();
};

// Relooper

Relooper::Relooper() : NumBlocks(0) {
}

Relooper::~Relooper() {
}

void Relooper::AddBlock(Block *NewBlock) {
  Blocks[NewBlock->Id] = NewBlock;
}

void Relooper::Render() {
  Shapes.reserve(Blocks.size()/2); // vague heuristic, better than nothing

  // Add incoming branches
  for (int i = 0; i < Blocks.size(); i++) {
    Block &Curr = *Blocks[i];
    for (int j = 0; j < Curr.BranchesOut.size(); j++) {
      Curr.BranchesOut[j]->BranchesIn.push_back(Curr);
    }
  }

  
}

