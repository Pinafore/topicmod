from collections import defaultdict
from math import log
from topicmod.util import flags
from PMI_statistics import *
from numpy import *
from scipy import linalg
import random


def normalizeCut(A, words_list):
  A = mat(A)
  col_sum = A.sum(axis=0)
  col_sum_nsqrt = array(col_sum)
  col_sum_nsqrt = col_sum_nsqrt ** (-0.5)
  words_num = len(A)

  D = zeros((words_num, words_num), float)
  D_nsqrt = zeros((words_num, words_num), float)

  for ii in range(words_num):
    D[ii, ii] = col_sum[0, ii]
    D_nsqrt[ii, ii] = col_sum_nsqrt[0, ii]

  A1 = A+D
  L = D_nsqrt * (D - A1) * D_nsqrt
  evalue, evector = linalg.eig(L)
  sorted_index = evalue.argsort()
  index = sorted_index[1]
  eval1 = evalue[index]
  evec1 = evector[0:words_num, index]

  print "eig values: \n"
  print evalue
  print "eig vectors: \n"
  print evector

  evec1_sorted = sort(evec1)
  thresh = (evec1_sorted[words_num-1] - evec1_sorted[0]) / 3
  thresh_low = evec1_sorted[0] + thresh
  thresh_high = evec1_sorted[words_num-1] - thresh

  merge1 = []
  merge2 = []

  for ii in range(len(evec1)):
    if evec1[ii] >= thresh_high:
      merge1.append(words_list[ii])
    elif evec1[ii] <= thresh_low:
      merge2.append(words_list[ii])

  print evec1
  print words_list
  print thresh_low, thresh_high
  print merge1, merge2
  
  split_pair = (merge1[0], merge2[0])

  return split_pair, merge1, merge2

def split_a_topic(cooccur, cooccur_total, tfidf, tfidf_thresh, topics, wordcount, wordcount_total):

  minimum = 100000
  split_topic = -1

  for tt in topics.keys():

    #tmp_words = set(topics[tt].keys())
    tmp_words = topics[tt]

    # Generate cannot links
    # If two words with very low cooccur appearing in the same topic
    # We should add a cannot link

    word_pairs = get_word_pairs(tmp_words, tmp_words)
    pmi_sum = 0
    count = 0
    for (w1, w2) in word_pairs:
      if tfidf[w1] > tfidf_thresh and tfidf[w2] > tfidf_thresh:
        pmi = computePMI2(cooccur[w1][w2], cooccur_total, wordcount[w1], wordcount[w2], wordcount_total)
        count += 1
        pmi_sum += pmi

    if count > 0:
      pmi_sum /= count
      if minimum > pmi_sum:
        minimum = pmi_sum
        split_topic = tt

  print "Splitting topic", split_topic
  assert split_topic != -1

  # split one topic
  tmp_words = topics[split_topic]

  tmp_list = defaultdict()
  tmp_list = list(tmp_words)
  #pmi_array = [0. for x in range(len(tmp_list))]
  #pmi_array = diag(pmi_array)
  pmi_array = zeros((len(tmp_list), len(tmp_list)), float)
  print pmi_array

  for ii in range(len(tmp_list)):
    for jj in range(ii+1, len(tmp_list)):
      w1 = tmp_list[ii]
      w2 = tmp_list[jj]
      if w1 < w2:
        pmi = computePMI2(cooccur[w1][w2], cooccur_total, wordcount[w1], wordcount[w2], wordcount_total)
      else:
        pmi = computePMI2(cooccur[w2][w1], cooccur_total, wordcount[w1], wordcount[w2], wordcount_total)
      pmi_array[ii, jj] = pmi
      pmi_array[jj, ii] = pmi
      print pmi_array[ii, jj], pmi_array[jj, ii]

  print pmi_array

  # for debug
  tmp = "["
  for ii in range(len(tmp_list)):
    tmp += "["
    for jj in range(len(tmp_list)):
      tmp += str(pmi_array[ii, jj]) + ", "
    tmp += "], "
  tmp += "]"
  print tmp

  [split_pair, merge1, merge2] = normalizeCut(pmi_array, tmp_list)

  return split_pair, merge1, merge2


def write_links(split_pair, merge1, merge2, output_dir):

  print "Generating the links!"
  output_file = open(output_dir, 'w')

  (w1, w2) = split_pair
  tmp = 'SPLIT_\t' + w1 + '\t' + w2 + '\n'
  output_file.write(tmp)

  tmp = 'MERGE_'
  for word in merge1:
    tmp += '\t' + word
  tmp += '\n'
  output_file.write(tmp)

  tmp = 'MERGE_'
  for word in merge2:
    tmp += '\t' + word
  tmp += '\n'
  output_file.write(tmp)

  output_file.close()


flags.define_string("vocab", "vocab/20_news.voc", "Where we find the vocab")
flags.define_string("stats", None, "Where we find the stat_file")
flags.define_string("model", "", "The model files folder of topic models")
flags.define_string("output", "constraints/tmp", "Output filename")
flags.define_int("topics_cutoff", 30, "Number of topic words")
flags.define_float("tfidf_thresh", 0, "threshold for tfidf")

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

  print "Reading statistics"
  [cooccur, cooccur_total, tfidf, wordcount, wordcount_total] \
                           = readStatistics(flags.stats, topic_word_set, vocab_size)

  print "Generating the links"
  [split_pair, merge1, merge2] = split_a_topic(cooccur, cooccur_total, tfidf, \
                        flags.tfidf_thresh, topics, wordcount, wordcount_total)

  print "Writing links to file"
  write_links(split_pair, merge1, merge2, flags.output)

