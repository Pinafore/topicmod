
from topicmod.util import flags
from topicmod.corpora.ontology_writer import OntologyWriter
import codecs

flags.define_string("source_words", None, "Where we read the vocab")
flags.define_string("output", None, "Where we write the new tree")

if __name__ == "__main__":
  flags.InitFlags()
  o = OntologyWriter(flags.output)

  words = set([])

  for ii in codecs.open(flags.source_words, encoding="utf-8"):
    lang = int(ii[:ii.find("\t")])
    word = ii[ii.find("\t"):].strip()
    words.add((lang, word, 1))

  #print words.keys(), words.values()
  root_id = 0
  o.AddSynset(root_id, "Root", [], words)
  o.Finalize()
