#!/usr/bin/python

import sys
from collections import defaultdict

if __name__ == "__main__":
    d = defaultdict(int)

    for ii in sys.stdin:
        d[ii.lower().strip()] += 1

    for ii in d:
        print("%05i\t%s" % (d[ii], ii))
