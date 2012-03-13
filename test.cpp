
#include "Relooper.h"

struct SimpleBlock : public Block {
  void Render() {
    PrintIndented("Block: %d\n", Id);

    // Normally we would connect branchings with conditions, etc., here we just show them simply
    for (BlockBranchMap::iterator iter = BranchesOut.begin(); iter != BranchesOut.end(); iter++) {
      iter->second->Render(iter->first);
    }
  }
};

int main() {
  /*
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
  */

  // Loop + tail pattern

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

