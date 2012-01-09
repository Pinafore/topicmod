#!/usr/bin/python

import sys

args = [x.rsplit("/")[-1] for x in sys.argv[1:]]
args.sort()

print ",".join(args)
