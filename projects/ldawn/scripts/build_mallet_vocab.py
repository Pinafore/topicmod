import gzip
import sys


class MalletAssignment:

  def __init__(self, line, debug=False):
    if debug:
      for ii in xrange(len(line.split())):
        print ii, line.split()[ii]
    self.doc, foo, self.index, self.term_id, self.term, self.assignment = \
      line.split()
    self.doc = int(self.doc)
    self.index = int(self.index)
    self.term_id = int(self.term_id)
    self.assignment = int(self.assignment)


if __name__ == "__main__":
  vocab = {}
  line_num = 0

  filename = sys.argv[1]
  infile = gzip.open(filename)
  for ii in infile:
    if ii.startswith("#"):
      continue

    line = MalletAssignment(ii)
    vocab[line.term_id] = line.term

    line_num += 1

  for ii in xrange(max(vocab.keys()) + 1):
    print "0\t%s" % vocab[ii]
