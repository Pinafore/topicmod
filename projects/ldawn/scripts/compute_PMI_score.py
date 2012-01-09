from collections import defaultdict
from math import log
from topicmod.util import flags
from PMI_statistics import *


def compute_PMI_score(cooccur, cooccur_total, wordcount, wordcount_total, topics, PMI_file):
  infile = open(PMI_file, 'w')

  total_pmi_score = 0
  for tt in topics.keys():
    tmp_words = topics[tt]
    pmi_score = 0
    word_pairs = get_word_pairs(tmp_words, tmp_words)
    for (w1, w2) in word_pairs:
      pmi = computePMI(cooccur[w1][w2], cooccur_total, wordcount[w1], wordcount[w2], wordcount_total)
      pmi_score += pmi

    pmi_score /= len(word_pairs)

    tmp = str(tt) + "\t" + str(len(word_pairs)) + "\t" + str(pmi_score) + "\n"
    infile.write(tmp)

    total_pmi_score += pmi_score

  total_pmi_score /= len(topics.keys())
  tmp = "total" + "\t" + str(len(topics.keys())) + "\t" + str(total_pmi_score) + "\n"
  infile.write(tmp)
  infile.close()


flags.define_string("vocab", "", "Where we find the vocab")
flags.define_string("model", "", "The model files folder of topic models")
flags.define_string("stats", None, "Where we find the stat_file")
flags.define_int("topics_cutoff", 30, "Number of topics")
flags.define_int("window_size", 10, "Size of window for computing coocurrance")
flags.define_string("output", "output/PMI_score", "PMI Output filename")

if __name__ == "__main__":

  flags.InitFlags()

  print "Reading vocab"
  [vocab_word_index, vocab_index_word] = readVocab(flags.vocab)
  vocab_size = len(vocab_word_index)

  print "Reading topic words"
  [topics, topic_word_set] = readTopics(flags.model, flags.topics_cutoff)

  #print "Get statistics"
  #[cooccur, wordcount, doc_num] = get_PMI_statistics(flags.corpus, topics,
  #                                topic_word_set, flags.window_size)

  # Note we don't actually need tfidf values
  #[cooccur, wordcount, tfidf, doc_num] = get_statistics(flags.corpus, flags.model, topics,
  #                                topic_word_set, flags.window_size, False)

  print "Reading statistics"
  [cooccur, cooccur_total, tfidf, wordcount, wordcount_total] \
                           = readStatistics(flags.stats, topic_word_set, vocab_size)

  print "Compute PMI score"
  compute_PMI_score(cooccur, cooccur_total, wordcount, wordcount_total, topics, flags.output)
