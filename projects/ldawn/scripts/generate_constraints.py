from collections import defaultdict
from math import log
from topicmod.util import flags
from PMI_statistics import *


def generate_links_PMI(cooccur, cN, tfidf, tfidf_thresh, topics,\
                       wordcount, wN):

  cannot = FreqDist()
  must = FreqDist()

  for tt in topics.keys():

    #tmp_words = set(topics[tt].keys())
    tmp_words = topics[tt]

    # Generate cannot links
    # If two words with very low cooccur appearing in the same topic
    # We should add a cannot link

    word_pairs = get_word_pairs(tmp_words, tmp_words)
    for (w1, w2) in word_pairs:
      if cooccur[w1][w2] > 0 and tfidf[w1] > tfidf_thresh \
                             and tfidf[w2] > tfidf_thresh:
        pmi = computePMI(cooccur[w1][w2], cN, wordcount[w1], wordcount[w2], wN)
        value = -1 * pmi
        cannot[(w1, w2)] = value

    # Generate must links
    # If two words with very high cooccur appearing in the different topics
    # We should add a must link
    topic_words = set()
    for tt_other in topics.keys():
      if tt_other != tt:
        # other_words = set(topics[tt_other].keys())
        other_words = topics[tt_other]
        shared = other_words.intersection(tmp_words)
        other_remained = other_words.difference(shared)
        tmp_remained = tmp_words.difference(shared)
        # print len(other_words), len(other_remained), \
        #  len(shared), len(tmp_words), len(tmp_remained)

        word_pairs = get_word_pairs(tmp_remained, other_remained)
        for (w1, w2) in word_pairs:
          if cooccur[w1][w2] > 0 and tfidf[w1] > tfidf_thresh \
                                 and tfidf[w2] > tfidf_thresh:
            pmi = computePMI(cooccur[w1][w2], cN, wordcount[w1], wordcount[w2], wN)
            value = pmi
            must[(w1, w2)] = value

  return cannot, must


def write_links(cannot, must, vocab, cannot_links_num, \
                must_links_num, output_dir):

  print "Generating the links!"
  output_file = open(output_dir, 'w')

  count = 0
  for (w1, w2) in cannot.keys():
    if count < cannot_links_num:
      pmi = cannot[(w1, w2)] * (-1)
      tmp = 'SPLIT_\t' + w1 + '\t' + w2 + '\n'
      output_file.write(tmp)
      count += 1

  count = 0
  for (w1, w2) in must.keys():
    if count < must_links_num:
      pmi = must[(w1, w2)]
      tmp = 'MERGE_\t' + w1 + '\t' + w2 + '\n'
      output_file.write(tmp)
      count += 1

  output_file.close()


flags.define_string("vocab", "vocab/20_news.voc", "Where we find the vocab")
flags.define_string("stats", "", "Where we find the stat_file")
flags.define_string("model", "", "The model files folder of topic models")
flags.define_string("output", "constraints/tmp", "Output filename")
flags.define_int("topics_cutoff", 30, "Number of topic words")
flags.define_int("cannot_links", 0, "Number of cannot links that we want")
flags.define_int("must_links", 0, "Number of must links that we want")

flags.define_int("num_topics", 20, "Number of topics")
flags.define_bool("train_only", False, "Using only train data to \
                                        generate the constraints")
flags.define_int("window_size", 10, "Size of window for computing coocurrance")
flags.define_float("tfidf_thresh", 0.0, "threshold for tfidf")

if __name__ == "__main__":

  flags.InitFlags()

  # getting statistics: slower version, full statistics, memory cost
  #[cooccur, wordcount, tfidf, vocab_index_word, topics, doc_num] \
  #      = get_statistics_all(flags.corpus, flags.num_topics, flags.model, \
  #             flags.topics_cutoff, flags.window_size, flags.train_only)


  # getting statistics: faster version, partial statistics, memory efficient
  print "Reading vocab"
  [vocab_word_index, vocab_index_word] = readVocab(flags.vocab)
  vocab_size = len(vocab_word_index)

  print "Reading topic words"
  [topics, topic_word_set] = readTopics(flags.model, flags.topics_cutoff)

  #print "Get statistics"
  #[cooccur, wordcount, tfidf, doc_num] = get_statistics(flags.corpus, flags.model, 
  #                    topics, topic_word_set, flags.window_size, flags.train_only)

  print "Reading statistics"
  [cooccur, cooccur_total, tfidf, wordcount, wordcount_total] \
                           = readStatistics(flags.stats, topic_word_set, vocab_size)

  print "Generating the links"
  [cannot, must] = generate_links_PMI(cooccur, cooccur_total, tfidf, flags.tfidf_thresh,\
                                      topics, wordcount, wordcount_total)

  print "Writing links to file"
  write_links(cannot, must, vocab_index_word, flags.cannot_links, flags.must_links, \
              flags.output)

