from hcluster import pdist, linkage, dendrogram
from numpy import *
import re
from numpy.random import rand
from topicmod.util import flags
from PMI_statistics import *


def findTopics(topics, cons_file_name):
  cons_file = open(cons_file_name, 'r')
  cons_words = []
  for cons in cons_file:
    if re.search("^SPLIT_", cons):
      cons = cons.strip()
      words = cons.split("\t")
      cons_words.append(words[1])
      cons_words.append(words[2])
    print cons_words

  topic_tmp = []
  for tt in topics.keys():
    count = 0
    for word in cons_words:
      if word in topics[tt]:
        count += 1
    if len(cons_words) == count:
      topic_tmp.append(tt)

  assert len(topic_tmp) == 1

  return topic_tmp[0], cons_words


def getPairDist(cooccur, cooccur_total, wordcount, wordcount_total, topic_words):

  tmp_words = topic_words

  word_list = list(tmp_words)
  sz = len(word_list) * (len(word_list) - 1) / 2
  p_dist = zeros(sz, float)

  index = -1
  for ii in range(len(word_list)):
    for jj in range(ii+1, len(word_list)):
      w1 = word_list[ii]
      w2 = word_list[jj]
      if w1 < w2:
        pmi = computePMI2(cooccur[w1][w2], cooccur_total, wordcount[w1], wordcount[w2], wordcount_total)
      else:
        pmi = computePMI2(cooccur[w2][w1], cooccur_total, wordcount[w1], wordcount[w2], wordcount_total)
      index += 1
      p_dist[index] = 1.0 / pmi

  print p_dist

  return p_dist, word_list


def findPath(Z, index, leaves_num):
  cn = leaves_num - 1
  word_path = []
  word_path.append(index)
  index_tmp = index
  for step in range(len(Z)):
    cn += 1
    if Z[step][0] == index_tmp or Z[step][1] == index_tmp:
      word_path.append(cn)
      index_tmp = cn

  return word_path


def findCluster(Z, cluster_root, word_list):
  cluster = []
  cluster.append(cluster_root)
  head = 0
  tail = 1
  while head < tail:
    tmp = cluster[head]
    if  tmp >= len(word_list):
      tmp -= len(word_list)
      cluster.append(Z[tmp][0])
      cluster.append(Z[tmp][1])
      tail += 2
    head += 1

  merge = []
  for ii in cluster:
    if ii < len(word_list):
      tmp = int(ii)
      merge.append(word_list[tmp])

  return merge


def hierarchicalClustering(p_dist, word_list, cons_words):

  Z = linkage(p_dist)

  index1 = word_list.index(cons_words[0])
  assert index1 >= 0
  path1 = findPath(Z, index1, len(word_list))
  index2 = word_list.index(cons_words[1])
  assert index2 >= 0  
  path2 = findPath(Z, index2, len(word_list))

  print Z
  print path1
  print path2

  common = set(path1).intersection(set(path2))
  # at least have the common root
  first = min(common)
  assert(first >= len(word_list))
  first -= len(word_list) 
  cluster_root = Z[first][0]
  merge1 = findCluster(Z, cluster_root, word_list)
  cluster_root = Z[first][1]
  merge2 = findCluster(Z, cluster_root, word_list)

  print word_list
  print merge1
  print merge2

  split_pair = (cons_words[0], cons_words[1])

  return split_pair, merge1, merge2


def extentCons(cooccur, cooccur_total, wordcount, wordcount_total, topic, cons_words):

  [p_dist, word_list] = getPairDist(cooccur, cooccur_total, wordcount, wordcount_total, topic)

  [split_pair, merge1, merge2] = hierarchicalClustering(p_dist, word_list, cons_words)

  return split_pair, merge1, merge2


def write_links(split_pair, merge1, merge2, output_dir):

  print "Generating the links!"
  output_file = open(output_dir, 'w')

  (w1, w2) = split_pair
  tmp = 'SPLIT_\t' + w1 + '\t' + w2 + '\n'
  output_file.write(tmp)

  if len(merge1) > 1:
    tmp = 'MERGE_'
    for word in merge1:
      tmp += '\t' + word
    tmp += '\n'
    output_file.write(tmp)

  if len(merge2) > 1:
    tmp = 'MERGE_'
    for word in merge2:
      tmp += '\t' + word
    tmp += '\n'
    output_file.write(tmp)

  output_file.close()

  

def test():
  word_list = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O' ]
  cons_words = ['C', 'B']
  X = rand(15, 2)
  #X = [[0.35, 0.37], [0.40, 0.40], [0.53, 0.53], [0.34, 0.51]]
  print X
  Y = pdist(X)
  print Y
  Z = linkage(Y)
  R = dendrogram(Z)

  index1 = word_list.index(cons_words[0])
  assert index1 >= 0
  path1 = findPath(Z, index1, len(word_list))
  index2 = word_list.index(cons_words[1])
  assert index2 >= 0  
  path2 = findPath(Z, index2, len(word_list))
  
  print Z
  print path1
  print path2

  common = set(path1).intersection(set(path2))
  first = min(common)
  assert(first >= len(word_list))
  first -= len(word_list) 
  cluster_root = Z[first][0]
  merge1 = findCluster(Z, cluster_root, word_list)
  cluster_root = Z[first][1]
  merge2 = findCluster(Z, cluster_root, word_list)

  print merge1
  print merge2


flags.define_string("vocab", "vocab/20_news.voc", "Where we find the vocab")
flags.define_string("stats", None, "Where we find the stat_file")
flags.define_string("model", "", "The model files folder of topic models")
flags.define_string("constraint", "constraints/tmp", "Original constraint file")
flags.define_int("topics_cutoff", 30, "Number of topic words")

if __name__ == "__main__":

  # test()

  flags.InitFlags()

  # getting statistics: faster version, partial statistics, memory efficient
  print "Reading vocab"
  [vocab_word_index, vocab_index_word] = readVocab(flags.vocab)
  vocab_size = len(vocab_word_index)

  print "Reading topic words"
  [topics, topic_word_set] = readTopics(flags.model, flags.topics_cutoff)

  print "Find the splitting topic"
  [tt, cons_words] = findTopics(topics, flags.constraint)

  print "Reading statistics"
  [cooccur, cooccur_total, tfidf, wordcount, wordcount_total] \
                          = readStatistics(flags.stats, topics[tt], vocab_size)

  print "Generating the links"
  [split_pair, merge1, merge2] = extentCons(cooccur, cooccur_total, wordcount, \
                                 wordcount_total, topics[tt], cons_words)

  print "Writing links to file"
  write_links(split_pair, merge1, merge2, flags.constraint + ".ext")
