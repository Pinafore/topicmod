
from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.util import flags
from topicmod.corpora.amazon import AmazonCorpus

flags.define_int("doc_limit", -1, "How many documents we add")
flags.define_list("langs", ["en"], "Which langauges do we add")
flags.define_string("base", "../../data/multiling-sent/", \
                      "Where we look for data")
flags.define_string("output", "/tmp/", "Where we write output")

LANGUAGE_CONSTANTS = {"en": ENGLISH, "de": GERMAN, "zh": CHINESE, \
                        "fr": FRENCH, "es": SPANISH, "ar": ARABIC}

if __name__ == "__main__":
  flags.InitFlags()
  corpus = AmazonCorpus(flags.base, flags.doc_limit)
  for ll in flags.langs:
    corpus.add_language("amzn-%s/*/*" % ll, LANGUAGE_CONSTANTS[ll])

  corpus.write_proto(flags.output + "numeric", "amazon")
