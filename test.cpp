
#include "Relooper.h"

struct SimpleBlock : public Block {
  void Render() {
    PrintIndented("// Block: %d\n", Id);

    int counter = 1000;
    for (BlockBranchMap::iterator iter = ProcessedBranchesOut.begin(); iter != ProcessedBranchesOut.end(); iter++) {
      PrintIndented("%sif (condition == %d) {\n", (counter == 1000) ? "" : "} else ", counter);
      counter++;
      Indenter::Indent();
      iter->second->Render(iter->first);
      Indenter::Unindent();
    }
    if (counter > 1000) PrintIndented("}\n");
  }
};

int main() {
  Debugging::On = 0;

  if (1) {
    printf("\n-- Diamond pattern --\n");

    SimpleBlock b1, b2, b3, b4;

    b1.AddBranchTo(&b2);
    b1.AddBranchTo(&b3);

    b2.AddBranchTo(&b4);

    b3.AddBranchTo(&b4);

    Relooper r;
    r.Blocks.push_back(&b1);
    r.Blocks.push_back(&b2);
    r.Blocks.push_back(&b3);
    r.Blocks.push_back(&b4);

    r.Calculate(&b1);
    r.Render();
  }

  if (1) {
    printf("\n-- Loop + tail pattern --\n");

    SimpleBlock b1, b2, b3;

    b1.AddBranchTo(&b2);

    b2.AddBranchTo(&b1);
    b2.AddBranchTo(&b3);

    Relooper r;
    r.Blocks.push_back(&b1);
    r.Blocks.push_back(&b2);
    r.Blocks.push_back(&b3);

    r.Calculate(&b1);
    r.Render();
  }
}

