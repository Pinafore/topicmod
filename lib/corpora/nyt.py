
from topicmod.corpora.nyt_reader import *
from topicmod.util import flags

flags.define_string("nyt_base", "../../data/new_york_times/",
                    "Where we find the nyt corpus")
flags.define_int("doc_limit", -1, "How many documents")
flags.define_string("output", "/tmp/jbg/nyt/", "Where we write data")
flags.define_float("bigram_limit", 0.9, "p-value for bigrams")

if __name__ == "__main__":
  flags.InitFlags()
  nyt = NewYorkTimesReader(flags.nyt_base, flags.doc_limit, flags.bigram_limit)
  nyt.add_language_list("../../data/new_york_times/editorial_file_list")

  nyt.write_proto(flags.output + "numeric", "nyt", 1000)
