
import codecs

from collections import defaultdict

from topicmod.util import flags
from topicmod.ling.dictionary import DingEntries

flags.define_string("vocab", "", "Where we read vocab")
flags.define_float("smoothing", 0.001, "Smoothing amount")
flags.define_float("hit", 1.0, "Value if there's a hit")
flags.define_string("output", "lda/lambda", "Lambda output")

if __name__ == "__main__":
  flags.InitFlags()

  vocab = defaultdict(dict)
  index = defaultdict(int)

  for ii in codecs.open(flags.vocab):
    lang, word = ii.split("\t")
    lang = int(lang)
    vocab[lang][word.strip()] = index[lang]
    index[lang] += 1

  trans = defaultdict(set)
  sum = defaultdict(float)
  for ii in vocab[0]:
    for jj in vocab[1]:
      if ii == jj:
        if vocab[1][jj] % 100 == 0:
          print "ID", jj, ii
        trans[vocab[1][jj]].add(vocab[0][ii])

  for en, de in DingEntries("en", "de"):
    for ii in en.words():
      if not ii in vocab[0]:
        continue
      for jj in de.words():
        if not jj in vocab[1]:
          continue
        if vocab[1][jj] % 100 == 0:
          print "DICT", jj, ii
        trans[vocab[1][jj]].add(vocab[0][ii])

  o = open(flags.output, 'w')
  vocab_size = [max(vocab[x].values()) + 1 for x in vocab]
  for jj in xrange(vocab_size[1]):
    sum = 0.0
    for ii in xrange(vocab_size[0]):
      val = flags.smoothing
      if ii in trans[jj]:
        val = flags.hit
      sum += val
    if jj % 100 == 0:
      print jj, sum
    for ii in xrange(vocab_size[0]):
      val = flags.smoothing
      if ii in trans[jj]:
        val = flags.hit
      o.write("%i\t%i\t%f\n" % (ii + 1, jj + 1, val / sum))

