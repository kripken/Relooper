#include "Relooper.h"

// Renderer

static int Renderer::CurrIndent = 0;

// MultipleShape

void MultipleShape::Render() {
    for (BlockIdShapeMap::iterator iter = InnerMap.begin(); iter != InnerMap.end(); iter++) {
      Renderer::Print"if (label == %d) {\n", iter->first);
      Renderer::Indent();
      iter->second->Render();
      Renderer::Unindent();
      Relooper::Print("}\n");
    }
    if (Next) Next->Render();
  }
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

