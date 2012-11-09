
#include "Relooper.h"

int main() {
  Debugging::On = 0;

  char buffer[10000];

  if (1) {
    Relooper::SetOutputBuffer(buffer, sizeof(buffer));

    printf("\n\n-- If pattern --\n\n");

    Block *b_a = new Block("// block A\n");
    Block *b_b = new Block("// block B\n");
    Block *b_c = new Block("// block C\n");

    b_a->AddBranchTo(b_b, "check == 10", "atob();");
    b_a->AddBranchTo(b_c, NULL, "atoc();");

    b_b->AddBranchTo(b_c, NULL, "btoc();");

    Relooper r;
    r.AddBlock(b_a);
    r.AddBlock(b_b);
    r.AddBlock(b_c);

    r.Calculate(b_a);
    printf("\n\n");
    r.Render();

    puts(buffer);
  }

  if (1) {
    Relooper::SetOutputBuffer(buffer, sizeof(buffer));

    printf("\n\n-- If-else pattern --\n\n");

    Block *b_a = new Block("// block A\n");
    Block *b_b = new Block("// block B\n");
    Block *b_c = new Block("// block C\n");
    Block *b_d = new Block("// block D\n");

    b_a->AddBranchTo(b_b, "check == 15");
    b_a->AddBranchTo(b_c, NULL);

    b_b->AddBranchTo(b_d, NULL);

    b_c->AddBranchTo(b_d, NULL);

    Relooper r;
    r.AddBlock(b_a);
    r.AddBlock(b_b);
    r.AddBlock(b_c);
    r.AddBlock(b_d);

    r.Calculate(b_a);
    printf("\n\n");
    r.Render();

    puts(buffer);
  }

  if (1) {
    Relooper::SetOutputBuffer(buffer, sizeof(buffer));

    printf("\n\n-- Loop + tail pattern --\n\n");

    Block *b_a = new Block("// block A\nvar check = maybe();\n");
    Block *b_b = new Block("// block B\n");
    Block *b_c = new Block("// block C\n");

    b_a->AddBranchTo(b_b, NULL);

    b_b->AddBranchTo(b_a, "check == 41");
    b_b->AddBranchTo(b_c, NULL);

    Relooper r;
    r.AddBlock(b_a);
    r.AddBlock(b_b);
    r.AddBlock(b_c);

    r.Calculate(b_a);
    printf("\n\n");
    r.Render();

    puts(buffer);
  }
}

