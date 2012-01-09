
import nltk
from nltk.corpus import brown

import os

if __name__ == "__main__":
  doc_num = 0

  os.mkdir("brown_sentences")

  for ii in brown.sents():
    o = open("brown_sentences/%i" % doc_num, 'w')
    o.write(" ".join(ii))
    doc_num += 1
