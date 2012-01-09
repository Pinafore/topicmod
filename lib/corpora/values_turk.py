from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.util import flags
#from topicmod.corpora.flat import FlatCorpus
from topicmod.corpora.flat import FlatCorpus

flags.define_int("doc_limit", -1, "How many documents we add")
flags.define_string("base", "../../data/values_turk/", \
                      "Where we look for data")
flags.define_string("output", "/tmp/", "Where we write output")

if __name__ == "__main__":
  flags.InitFlags()
  #corpus = FlatCorpus(flags.base, flags.doc_limit)
  corpus = FlatCorpus(flags.base, flags.doc_limit)
  corpus.add_language("1/*")
  corpus.add_language("2/*")

  corpus.write_proto(flags.output + "numeric", "values_turk")
