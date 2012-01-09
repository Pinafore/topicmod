
from topicmod.util import flags
from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.corpora.proto.wordnet_file_pb2 import *
from topicmod.corpora.ml_vocab import Vocab
from topicmod.corpora.ml_vocab import MultilingualVocab

import codecs

from collections import defaultdict

flags.define_glob("wordnet", "", "The wordnet files")
flags.define_string("vocab", "", "The vocab file")
flags.define_glob("docs", "", "The documents we want to view")
flags.define_glob("doc_roots", "", "The document vocab")


def print_doc(filename, full, flat, wn):
  doc = Document()
  doc.ParseFromString(open(filename, 'rb').read())

  print "-------------------------------"
  print "Original document:"
  for sent in doc.sentences:
    for word in sent.words:
      print "|%i:%i:%s|" % (doc.language, word.token, \
                              full.get_word(doc.language, \
                                              word.token).encode("ascii", \
                                                                   'ignore')),
  print ""

  print "Vocab filtered:"
  for sent in doc.sentences:
    for word in sent.words:
      s = full.get_word(doc.language, word.token)
      if flat.get_id(doc.language, s) != -1:
        print s.encode("ascii", "ignore"),
  print ""

  print "WN filtered:"
  for sent in doc.sentences:
    for word in sent.words:
      s = full.get_word(doc.language, word.token)
      if s in wn[doc.language]:
        print s.encode("ascii", "ignore"),
  print ""

  print "Translated:"
  for sent in doc.sentences:
    for word in sent.words:
      new_word = full.get_word(doc.language, word.token)
      new_id = flat.get_id(doc.language, new_word)
      if new_id != -1:
        if new_word in wn[doc.language]:
          print "*%i" % new_id,
        else:
          print new_id,
  print ""

if __name__ == "__main__":
  flags.InitFlags()

  # Get the corpus vocab
  superset_vocab = MultilingualVocab()
  for root in flags.doc_roots:
    print "Reading root", root
    corpus = Corpus()
    f = open(root, "rb")
    corpus.ParseFromString(f.read())

    for ii in corpus.tokens:
      for jj in ii.terms:
        superset_vocab.set(ii.language, jj.id, jj.original)

  # Get the culled vocab
  filtered_vocab = MultilingualVocab(flags.vocab)

  # Get the wn vocab, limited by the vocabulary
  wn_vocab = defaultdict(set)
  for ii in flags.wordnet:
    wn = WordNetFile()
    print "Reading wordnet", ii
    f = open(ii, "rb")
    wn.ParseFromString(f.read())
    for synset in wn.synsets:
      for word in synset.words:
        if filtered_vocab.get_id(word.lang_id, word.term_str) != -1:
          wn_vocab[word.lang_id].add(word.term_str)

  for ii in wn_vocab:
    print ii, wn_vocab[ii]

  for ii in flags.docs:
    print_doc(ii, superset_vocab, filtered_vocab, wn_vocab)
