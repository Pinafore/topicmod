
from topicmod.corpora.wacky import *
from topicmod.util import flags

flags.define_string("wackypedia_base", "../../data/wackypedia/compressed/",
                    "Where we find the wackypedia corpus")
flags.define_string("output", "/tmp/jbg/wackypedia/", "Where we write output")
flags.define_int("doc_limit", 10, "Max number of docs")
flags.define_list("langs", ["en"], "Which languages")

if __name__ == "__main__":
  flags.InitFlags()
  wacky = WackyCorpus(flags.wackypedia_base, flags.doc_limit)
  for ii in flags.langs:
    wacky.add_language("wackypedia_%s*.gz" % ii)

  wacky.write_proto(flags.output + "numeric",
                    "wpdia", 10000)
