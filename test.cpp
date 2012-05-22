
#include "Relooper.h"

int main() {
  Debugging::On = 0;

  if (1) {
    printf("\n\n-- If pattern --\n\n");

    Block *b_a = new Block("// block A\n");
    Block *b_b = new Block("// block B\n");
    Block *b_c = new Block("// block C\n");

    b_a->AddBranchTo(b_b, "check == 10");
    b_a->AddBranchTo(b_c, "check == 20");

    b_b->AddBranchTo(b_c, "check == 30");

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

    Block *b_a = new Block("// block A\n");
    Block *b_b = new Block("// block B\n");
    Block *b_c = new Block("// block C\n");
    Block *b_d = new Block("// block D\n");

    b_a->AddBranchTo(b_b, "check == 15");
    b_a->AddBranchTo(b_c, "check == 25");

    b_b->AddBranchTo(b_d, "check == 35");

    b_c->AddBranchTo(b_d, "check == 45");

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

    Block *b_a = new Block("// block A\nvar check = maybe();\n");
    Block *b_b = new Block("// block B\n");
    Block *b_c = new Block("// block C\n");

    b_a->AddBranchTo(b_b, "check == 40");

    b_b->AddBranchTo(b_a, "check == 41");
    b_b->AddBranchTo(b_c, "check == 42");

    Relooper r;
    r.AddBlock(b_a);
    r.AddBlock(b_b);
    r.AddBlock(b_c);

    r.Calculate(b_a);
    printf("\n\n");
    r.Render();
  }
}

