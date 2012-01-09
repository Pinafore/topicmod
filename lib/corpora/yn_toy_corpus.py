from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.util import flags
from topicmod.corpora.flat import FlatCorpus
#from topicmod.corpora.flat import FlatEmailCorpus

flags.define_int("doc_limit", -1, "How many documents \
                                   we add")
flags.define_string("base", "../../data/yn_toy/", \
                      "Where we look for data")
flags.define_string("output", "../../data/yn_toy/numeric", \
                      "Where we write output")

if __name__ == "__main__":
  flags.InitFlags()
  corpus = FlatCorpus(flags.base, flags.doc_limit)
  corpus.add_language("*", ENGLISH)
  print flags.output

  corpus.write_proto(flags.output, "yn_toy")
