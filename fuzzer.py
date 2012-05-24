
import random, subprocess

while True:
  # Generate random block graph
  num = random.randint(1, 20)
  density = random.random()
  print num, density

  src = '''

#include <stdlib.h>
#include "Relooper.h"

int main() {
  Debugging::On = 0;

  char *buffer = (char*)malloc(10*1024*1024);
  Relooper::SetOutputBuffer(buffer);
'''

  for i in range(num):
    if i == 0:
      src += '''
  Block *b%d = new Block("print('entry'); var label; var decisions = %s; var index = 0; function check() { if (index == decisions.length) throw 'HALT'; return decisions[index++] }");
''' % (i, str([random.randint(0, 1) for x in range(1000)]))
    else:
      src += '''  Block *b%d = new Block("check(); print(%d);");
''' % (i, i)

  for i in range(num):
    for j in range(1, num):
      if random.random() <= density:
        src += '''  b%d->AddBranchTo(b%d, "check()");
''' % (i, j)

  src += '''
  Relooper r;
'''

  for i in range(num):
    src += '''  r.AddBlock(b%d);
''' % i

  src += '''
  r.Calculate(b0);
  printf("\\n\\n");
  r.Render();

  puts(buffer);

  return 1;
}
'''

  open('fuzz.cpp', 'w').write(src)

  subprocess.call(['g++', 'fuzz.cpp', 'Relooper.o', '-o', 'fuzz', '-g'])
  subprocess.call(['./fuzz'], stdout=open('fuzz.opt.js', 'w'))
  subprocess.call(['mozjs', '-m', '-n', 'fuzz.opt.js'], stdout=open('fuzz.opt.txt', 'w'))

  break

