
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

    b1.BranchesOut[&b2] = new Branch;
    b1.BranchesOut[&b3] = new Branch;

    b2.BranchesOut[&b4] = new Branch;

    b3.BranchesOut[&b4] = new Branch;

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

    b1.BranchesOut[&b2] = new Branch;

    b2.BranchesOut[&b1] = new Branch;
    b2.BranchesOut[&b3] = new Branch;

    Relooper r;
    r.Blocks.push_back(&b1);
    r.Blocks.push_back(&b2);
    r.Blocks.push_back(&b3);

    r.Calculate(&b1);
    r.Render();
  }
}

