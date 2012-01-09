
from topicmod.util import flags
from topicmod.corpora.vocab_compiler import VocabCompiler

flags.define_glob("corpus_parts", None, "Where we look for vocab")
flags.define_filename("output", None, "Where we write the new vocab")
flags.define_int("min_freq", 10, "Minimum frequency for inclusion")
flags.define_int("vocab_limit", 5000, "Maximum vocab size")
flags.define_bool("exclude_stop", True, "Do we throw out stop words")
flags.define_bool("exclude_punc", True, "Do we exclude punctuation")
flags.define_bool("exclude_digits", True, "Do we exclude digits")
flags.define_list("special_stop", [], "Special stop words")
flags.define_int("min_length", 3, "Minimum length for tokens")
flags.define_bool("stem", False, "Stem words")

if __name__ == "__main__":
  flags.InitFlags()
  v = VocabCompiler()
  for ii in flags.corpus_parts:
    print ii
    v.addVocab(ii, flags.exclude_stop, flags.special_stop, \
                 flags.exclude_punc, flags.exclude_digits, \
                 flags.stem, flags.min_length)
  v.writeVocab(flags.output, flags.vocab_limit, flags.min_freq)
