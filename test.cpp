
#include "Relooper.h"

struct SimpleBlock : public Block {
  void Render() {
    PrintIndented("Block: %d\n", Id);

    // Normally we would connect branchings with conditions, etc., here we just show them simply
    for (int i = 0; i < BranchesOut.size(); i++) {
      BranchesOut[i]->Render();
    }
  }
};

int main() {
  // Diamond pattern

  SimpleBlock b1, b2, b3, b4;

  b1.BranchesOut.push_back(new Branch(&b2));
  b1.BranchesOut.push_back(new Branch(&b3));

  b2.BranchesOut.push_back(new Branch(&b4));

  b3.BranchesOut.push_back(new Branch(&b4));

  Relooper r;
  r.Blocks.push_back(&b1);
  r.Blocks.push_back(&b2);
  r.Blocks.push_back(&b3);
  r.Blocks.push_back(&b4);

  r.Render();
}

