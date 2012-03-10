#include "Relooper.h"

// Renderer

static int Renderer::CurrIndent = 0;

// MultipleShape

void MultipleShape::Render() {
    for (BlockShapeMap::iterator iter = InnerMap.begin(); iter != InnerMap.end(); iter++) {
      Renderer::Print"if (label == %d) {\n", iter->first);
      Renderer::Indent();
      iter->second->Render();
      Renderer::Unindent();
      Relooper::Print("}\n");
    }
    if (Next) Next->Render();
  }
};

