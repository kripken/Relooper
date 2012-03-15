// js -m -n -e "load('relooper.js')" test.js

print("-- If shape --\n");

var b_a = new Block("// block A\n", "check");
var b_b = new Block("// block B\n", "check");
var b_c = new Block("// block C\n", "check");

b_a.AddBranchTo(b_b, 10);
b_a.AddBranchTo(b_c, 20);

b_b.AddBranchTo(b_c, 30);

var r = new Relooper();

r.AddBlock(b_a);
r.AddBlock(b_b);
r.AddBlock(b_c);

r.Calculate(b_a);
r.Render();

destroy(r);

