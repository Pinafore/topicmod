#!/usr/bin/python

import sys

args = sys.argv[1:]
args.sort()

print ",".join(args)
