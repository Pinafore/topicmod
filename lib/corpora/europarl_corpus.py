
from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.util import flags
from topicmod.corpora.europarl import EuroparlCorpus

flags.define_string("base", "../../data/europarl/", "Where we look for data")
flags.define_string("output", "/tmp/", "Where we write output")
flags.define_int("doc_limit", -1, "How many documents we add")

if __name__ == "__main__":
  flags.InitFlags()

  for ii in xrange(96, 107):
    year = ii % 100
    corpus = EuroparlCorpus(flags.base, flags.doc_limit)
    corpus.add_language("english/ep-%02i-*.en" % year, ENGLISH)
    corpus.add_language("german/ep-%02i-*.de" % year, GERMAN)

    corpus.write_proto(flags.output + "numeric", "europarl%02i" % year, 1000)
