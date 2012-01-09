
from topicmod.corpora.semcor import SemcorCorpus
from topicmod.util import flags

flags.define_string("semcor_base", "../../data/semcor-%s/", \
                      "Where we find the semcor corpus")
flags.define_string("wordnet_base", "../../data/wordnet/", \
                      "Where we find the wordnet corpus")
flags.define_string("version", "3.0", "Version of WordNet used")
flags.define_string("semcor_output", None, "Where we write the output")

if __name__ == "__main__":
  flags.InitFlags()
  semcor = SemcorCorpus(flags.semcor_base % flags.version)

  semcor.load_wn(flags.wordnet_base, flags.version)
  semcor.add_language("brown1/tagfiles/*")
  semcor.add_language("brown2/tagfiles/*")
  #semcor.add_language("brownv/tagfiles/br-e*")

  semcor.write_proto(flags.semcor_output, "semcor", 80)
