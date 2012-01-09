
from topicmod.util import flags
from topicmod.util.sets import count_line
from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.corpora.proto.wordnet_file_pb2 import *
from topicmod.corpora.ml_vocab import MultilingualVocab
from topicmod.corpora.ml_vocab import Vocab

from collections import defaultdict
import codecs

flags.define_string("output", "", "Where we write output")
flags.define_glob("doc_roots", "", "The document vocab")
flags.define_string("vocab", "", "The vocab file")
flags.define_string("location", "", "Where the data live")
flags.define_int("min_length", 50, "Minimum number of tokens")
flags.define_int("num_docs", -1, "Number of documents we write")
flags.define_string("language", "en", "What language this is")

kLANGUAGE_ID = {"en": ENGLISH, "de": GERMAN, "zh": CHINESE}


def lda_line(filename, full_vocab, filtered_vocab):
  d = defaultdict(int)

  doc = Document()
  doc.ParseFromString(open(filename, 'rb').read())

  num_words = 0
  for sent in doc.sentences:
    for word in sent.words:
      new_word = full_vocab.get_word(doc.language, word.token)
      if new_word in filtered_vocab:
          new_id = filtered_vocab[new_word]
          d[new_id] += 1
          num_words += 1

  return doc.rating, count_line(d), doc.title, num_words

if __name__ == "__main__":
  flags.InitFlags()

  # Get the corpus vocab
  o1 = open(flags.output + ".lda", 'w')
  o2 = open(flags.output + ".rat", 'w')
  o3 = open(flags.output + ".doc", 'w')

  num_docs = 0
  skipped = 0

  filter_vocab = Vocab(flags.vocab, kLANGUAGE_ID[flags.language])
  for root in flags.doc_roots:
    print "Reading root", root
    corpus = Corpus()
    f = open(root, "rb")
    corpus.ParseFromString(f.read())

    # This allows possibly inconsistent ways of assigning numbers of words, but
    # the final voc should make them consistent
    superset_vocab = MultilingualVocab()
    for ii in corpus.tokens:
      for jj in ii.terms:
        # print ii.language, jj.id, jj.original
        superset_vocab.set(ii.language, jj.id, jj.original)

    for ii in corpus.doc_filenames:
        rating, line, title, num_words = lda_line(flags.location + ii, \
                                                    superset_vocab, \
                                                    filter_vocab)
        if num_words >= flags.min_length:
          num_docs += 1
          o1.write(line)
          o2.write("%i\n" % rating)
          o3.write("%s\n" % title)
        else:
          skipped += 1
          #print skipped, num_docs, ii, num_words
        if flags.num_docs > 0 and num_docs >= flags.num_docs:
          break
  o1.close()
  o2.close()

  filter_vocab.write(flags.output + ".voc")
