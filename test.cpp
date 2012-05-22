
#include "Relooper.h"

int main() {
  Debugging::On = 0;

  if (1) {
    printf("\n\n-- If pattern --\n\n");

    Block *b_a = new Block("// block A\n", "check");
    Block *b_b = new Block("// block B\n", "check");
    Block *b_c = new Block("// block C\n", "check");

    b_a->AddBranchTo(b_b, "== 10");
    b_a->AddBranchTo(b_c, "== 20");

    b_b->AddBranchTo(b_c, "== 30");

    Relooper r;
    r.AddBlock(b_a);
    r.AddBlock(b_b);
    r.AddBlock(b_c);

    r.Calculate(b_a);
    printf("\n\n");
    r.Render();
  }

  if (1) {
    printf("\n\n-- If-else pattern --\n\n");

    Block *b_a = new Block("// block A\n", "check");
    Block *b_b = new Block("// block B\n", "check");
    Block *b_c = new Block("// block C\n", "check");
    Block *b_d = new Block("// block D\n", "check");

    b_a->AddBranchTo(b_b, "== 15");
    b_a->AddBranchTo(b_c, "== 25");

    b_b->AddBranchTo(b_d, "== 35");

    b_c->AddBranchTo(b_d, "== 45");

    Relooper r;
    r.AddBlock(b_a);
    r.AddBlock(b_b);
    r.AddBlock(b_c);
    r.AddBlock(b_d);

    r.Calculate(b_a);
    printf("\n\n");
    r.Render();
  }

  if (1) {
    printf("\n\n-- Loop + tail pattern --\n\n");

    Block *b_a = new Block("// block A\nvar check = maybe();\n", "check");
    Block *b_b = new Block("// block B\n", "check");
    Block *b_c = new Block("// block C\n", "check");

    b_a->AddBranchTo(b_b, "== 40");

    b_b->AddBranchTo(b_a, "== 41");
    b_b->AddBranchTo(b_c, "== 42");

    Relooper r;
    r.AddBlock(b_a);
    r.AddBlock(b_b);
    r.AddBlock(b_c);

    r.Calculate(b_a);
    printf("\n\n");
    r.Render();
  }
}

