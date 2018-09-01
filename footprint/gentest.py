import sys
import re
import random

def gencode():
    template = """int testXXX(int *val1, int *val2){
  return *val1 + *val2;
}
"""

    r = re.compile("XXX");
    for i in range(0,37449):
        l = r.sub(str(i), template);
        print l
    print "char * table = {&test0"
    f = range(1,37449);
    random.shuffle(f);
    for i in f:
        print ",&test%d" % i;
    print "};"

if __name__ == "__main__":
    usage = "python gentest size random\n"
    gencode()
