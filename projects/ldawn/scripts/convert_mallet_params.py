import gzip
import sys
import math

PARAM_NAMES = ["alpha", "lambda", "choice", "transition", "emission"]
DEFAULT_PARAMETER_VALUES = {"alpha": 0.05, "lambda": 0.01, "choice": 1.0, \
                                "transition": 0.01, "emission": 0.01}
PARAMETER_MAP = {"#alpha :": "alpha", "#beta :": "transition"}

if __name__ == "__main__":
  vocab = {}
  line_num = 0
  params = DEFAULT_PARAMETER_VALUES

  filename = sys.argv[1]
  infile = gzip.open(filename)

  line_num = 0
  for ii in infile:
    line_num += 1
    if ii.startswith("#"):
      for jj in PARAMETER_MAP:
        if ii.startswith(jj):
          val = ii.replace(jj, "").strip()
          val = val.split()[0]
          params[PARAMETER_MAP[jj]] = float(val)
    if line_num > 1000:
      break
  infile.close()

  for ii in PARAM_NAMES:
    print math.log(params[ii]),
  # print ""

  filename = sys.argv[2]
  infile = open(filename)
  num = infile.readline()
  for ii in infile:
    ww = ii.split(" ")
    for pp in ww[1:]:
      pp = float(pp)
      print math.log(pp),
  print ""
  infile.close()
