
import random, subprocess

while True:
  # Random decisions
  num = random.randint(1, 6)
  density = random.random()
  decisions = [random.randint(0, 1) for x in range(1000)]
  print num, density

  # parts
  entry = '''print('entry'); var label; var decisions = %s; var index = 0; function check() { if (index == decisions.length) throw 'HALT'; return decisions[index++] }''' % str(decisions)

  opt = '''

#include <stdlib.h>
#include "Relooper.h"

int main() {
  Debugging::On = 0;

  char *buffer = (char*)malloc(10*1024*1024);
  Relooper::SetOutputBuffer(buffer);
'''

  for i in range(num):
    if i == 0:
      opt += '''
  Block *b%d = new Block("%s");
''' % (i, entry)
    else:
      opt += '''  Block *b%d = new Block("check(); print(%d);");
''' % (i, i)

  for i in range(num):
    for j in range(1, num):
      if random.random() <= density:
        opt += '''  b%d->AddBranchTo(b%d, "check()");
''' % (i, j)

  opt += '''
  Relooper r;
'''

  for i in range(num):
    opt += '''  r.AddBlock(b%d);
''' % i

  opt += '''
  r.Calculate(b0);
  printf("\\n\\n");
  r.Render();

  puts(buffer);

  return 1;
}
'''

  open('fuzz.cpp', 'w').write(opt)

  subprocess.call(['g++', 'fuzz.cpp', 'Relooper.o', '-o', 'fuzz', '-g'])
  subprocess.call(['./fuzz'], stdout=open('fuzz.opt.js', 'w'))
  subprocess.call(['mozjs', '-m', '-n', 'fuzz.opt.js'], stdout=open('fuzz.opt.txt', 'w'))

  break

