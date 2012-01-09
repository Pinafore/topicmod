from collections import defaultdict
from nltk.probability import FreqDist
from glob import glob
from math import log
from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.corpora.proto.wordnet_file_pb2 import *

SMOOTH_FACTOR = 0.01

def readVocab(vocabname):
  vocabfile = open(vocabname, 'r')
  vocab_word_index = defaultdict()
  vocab_index_word = []
  index = -1
  for line in vocabfile:
    line = line.strip()
    words = line.split('\t')
    word = words[1]
    index += 1
    vocab_word_index[word] = index
    vocab_index_word.append(word)

  vocabfile.close()
  return vocab_word_index, vocab_index_word


def computePMI(cooccur, cooccur_total, wordcount1, wordcount2, wordcount_total):
  # PMI = P(x,y) / (P(x)P(y))
  #    = (cooccur(x,y) / N2) / ((wordcount(x) / N1) * (wordcount(y) / N1))
  pmi = cooccur * 1.0 * wordcount_total * wordcount_total \
        / cooccur_total / wordcount1 / wordcount2
  #pmi = log(pmi)
  return pmi


def computePMI2(cooccur, cooccur_total, wordcount1, wordcount2, wordcount_total):
  # PMI = P(x,y)^2 / (P(x)P(y))
  #    = (cooccur(x,y) / N2)^2 / ((wordcount(x) / N1) * (wordcount(y) / N1))
  pmi = cooccur * cooccur * 1.0 * wordcount_total * wordcount_total \
        / cooccur_total / cooccur_total / wordcount1 / wordcount2
  # pmi = log(pmi)
  return pmi


def readTopicsIndex(modelname, topics_cutoff, vocab_word_index):
  topicname = modelname + '.topics'
  topicfile = open(topicname, 'r')
  topics = defaultdict()
  topic_word_set = set()
  flag = 0
  topic_count = -1
  for line in topicfile:
    line = line.strip()
    if flag == 0 and line.find("Ontology multinomial") >= 0:
      flag = 1
      word_count = 0
      topic_count += 1
      topics[topic_count] = set()
    elif flag == 1 and word_count < topics_cutoff:
      words = line.split(' ')
      assert len(words) == 4
      word = words[3]
      index = vocab_word_index[word]
      topics[topic_count].add(index)
      topic_word_set.add(index)
      word_count += 1
    elif flag == 1 and word_count == topics_cutoff:
      flag = 0

  #for ii in topics.keys():
  #  print "topic ", ii,
  #  print "topic words: ", len(topics[ii]), " ", topics[ii]

  return topics, topic_word_set


def readTopics(modelname, topics_cutoff):
  topicname = modelname + '.topics'
  topicfile = open(topicname, 'r')
  topics = defaultdict()
  topic_word_set = set()
  flag = 0
  topic_count = -1
  for line in topicfile:
    line = line.strip()
    if flag == 0 and line.find("Ontology multinomial") >= 0:
      flag = 1
      word_count = 0
      topic_count += 1
      topics[topic_count] = set()
    elif flag == 1 and word_count < topics_cutoff:
      words = line.split(' ')
      assert len(words) == 4
      word = words[3]
      topics[topic_count].add(word)
      topic_word_set.add(word)
      word_count += 1
    elif flag == 1 and word_count == topics_cutoff:
      flag = 0

  #for ii in topics.keys():
  #  print "topic ", ii,
  #  print "topic words: ", len(topics[ii]), " ", topics[ii]

  return topics, topic_word_set


def get_word_pairs(words_set1, words_set2):
  pairs = set()

  for w1 in words_set1:
    for w2 in words_set2:
      if w1 < w2:
        pairs.add((w1, w2))
      elif w1 > w2:
        pairs.add((w2, w1))

  return pairs


# read topic words from model.topics
# only get statistics of topic words
# fast version 
# !!memory efficient!!
def get_statistics(corpus_dir, model_dir, topics,
                   topic_word_set, window_size, train_only):

  cooccur = defaultdict()
  wordcount = defaultdict()
  tfidf = defaultdict()
  topics = defaultdict()

  # Initialization
  for w1 in topic_word_set:
    wordcount[w1] = SMOOTH_FACTOR
    cooccur[w1] = defaultdict()
    tfidf[w1] = 0
    for w2 in topic_word_set:
      if w1 < w2:
        cooccur[w1][w2] = SMOOTH_FACTOR

  print "Initialized"

  if train_only:
    doc_id = open(model_dir + '.doc_id', 'r')

  doc_num = 0
  for ii in glob("%s/*.index" % corpus_dir):
    inputfile = open(ii, 'rb')
    protocorpus = Corpus()
    protocorpus.ParseFromString(inputfile.read())

    for dd in protocorpus.doc_filenames:

      doc_num += 1
      if doc_num % 1000 == 0:
        print "Finished reading", doc_num, "documents."

      docfile = open("%s/%s" % (corpus_dir, dd), 'rb')
      doc = Document()
      doc.ParseFromString(docfile.read())

      if train_only:
        doc_id_line = doc_id.readline()
        is_train = doc_id_line.find('  train/')
        if is_train < 0:
          continue

      words = []
      for jj in doc.sentences:
        # print "each sentence: ", jj.words
        for kk in jj.words:
          words.append(kk.token)
          # max tfidf
          if kk.token in topic_word_set and kk.tfidf > tfidf[kk.token]:
            tfidf[kk.token] = kk.tfidf

      words_len = len(words)
      for index1 in range(0, words_len):
        w1 = words[index1]
        if w1 in topic_word_set:
          wordcount[w1] += 1

          if window_size == -1:
            index_end = words_len
          else:
            index_end = min(words_len, index1 + window_size)

          for index2 in range(index1 + 1, index_end):
            w2 = words[index2]
            if w2 in topic_word_set:
              if w1 < w2:
                cooccur[w1][w2] += 1
              elif w1 > w2:
                cooccur[w2][w1] += 1

      #if doc_num > 10:
      #  break

    inputfile.close()

  if train_only:
    doc_id.close()

  return cooccur, wordcount, tfidf, doc_num


# read topic assignments from model.topic_assignments, 
# then decide the topic words according to topics_cutoff
# get statistics of all words, not only the topic words
# slower version 
# !!need much larger memory!!
def get_statistics_all(corpus_dir, num_topcis, model_dir,
          topics_cutoff, window_size, train_only):

  cooccur = defaultdict()
  wordcount = defaultdict()
  tfidf = defaultdict()
  tfidf_res = defaultdict()
  topics = defaultdict()

  read_voc = 0

  topic_assignments = open(model_dir + '.topic_assignments', 'r')
  doc_id = open(model_dir + '.doc_id', 'r')

  doc_num = 0

  for ii in glob("%s/*.index" % corpus_dir):
    inputfile = open(ii, 'rb')
    protocorpus = Corpus()
    protocorpus.ParseFromString(inputfile.read())

    if read_voc == 0:
      # assume that vocab in each index file is the same, so
      # we just need to load it once and initialize index once
      # or it is too slow
      read_voc = 1
      vocab = defaultdict()
      for lang in protocorpus.tokens:
        for ii in lang.terms:
          vocab[ii.id] = ii.original
          cooccur[ii.id] = defaultdict()
          wordcount[ii.id] = SMOOTH_FACTOR
          tfidf[ii.id] = 0
          #tfidf[ii.id] = []

      # Initialization
      for tt in range(0, num_topcis):
        topics[tt] = FreqDist()

      for w in cooccur.keys():
        for ww in vocab.keys():
          cooccur[w][ww] = SMOOTH_FACTOR

    for dd in protocorpus.doc_filenames:
      doc_num += 1
      if doc_num % 1000 == 0:
        print "Finished reading", doc_num, "documents."

      docfile = open("%s/%s" % (corpus_dir, dd), 'rb')
      doc = Document()
      doc.ParseFromString(docfile.read())

      topic_line = topic_assignments.readline()
      assign = topic_line.split(' ')
      # the first one is the number of words, not topic assignments
      ww_index = 0

      doc_id_line = doc_id.readline()
      is_train = doc_id_line.find('  train/')

      if train_only and is_train < 0:
        continue

      words = []
      for jj in doc.sentences:
        for kk in jj.words:
          ww_index += 1
          tt = int(assign[ww_index])
          topics[tt].inc(kk.token)

          # MAX tfidf
          if kk.tfidf > tfidf[kk.token]:
            tfidf[kk.token] = kk.tfidf
          # Mean tfidf
          # tfidf[kk.token] += kk.tfidf
          # Median tfidf
          # tfidf[kk.token].append(kk.tfidf)

          words.append(kk.token)

      words_len = len(words)
      for index1 in range(0, words_len):
        w1 = words[index1]
        wordcount[w1] += 1

        if window_size == -1:
          index_end = words_len
        else:
          index_end = min(words_len, index1 + window_size)

        for index2 in range(index1 + 1, index_end):
          w2 = words[index2]
          if w1 < w2:
            cooccur[w1][w2] += 1
          elif w1 > w2:
            cooccur[w2][w1] += 1

      #if doc_num > 10:
      #  break

    inputfile.close()

  topic_assignments.close()
  doc_id.close()

  for ww in tfidf.keys():
    if tfidf[ww] > 0:
      # max
      tfidf_res[ww] = tfidf[ww]
      # mean
      #tfidf_res[ww] = tfidf[ww] / wordcount[ww]
    # median
    #if len(tfidf[ww]) > 0:
    #  tmp = tfidf[ww].sort()
    #  mid = int(len(tfidf[ww]) / 2)
    #  tfidf_res[ww] = tfidf[ww][mid]

  #min_cooccur = 10000
  #max_cooccur = 0
  #for ww in cooccur.keys():
  #  min_cooccur = min(min_cooccur, min(cooccur[ww].values()))
  #  max_cooccur = max(max_cooccur, max(cooccur[ww].values()))
  #print min_cooccur, max_cooccur

  # choose the top words in each topic
  topics_top = defaultdict()
  for tt in topics.keys():
    count = 0
    topics_top[tt] = set()
    for ww in topics[tt].keys():
      if count < topics_cutoff:
        topics_top[tt].add(ww)
        count += 1
      else:
        break
    # print "Number of words in topic " + str(tt) + ": "
    #                                   + str(len(topics_top[tt]))

  return cooccur, wordcount, tfidf_res, vocab, topics_top, doc_num


# read tfidf of vocab words
def get_tfidf(proto_corpus_dir, vocab_file, output_dir):

  tfidf = defaultdict()

  [vocab_word_index, vocab_index_word] = readVocab(vocab_file)

  doc_num = 0
  for ii in glob("%s/*.index" % proto_corpus_dir):
    inputfile = open(ii, 'rb')
    protocorpus = Corpus()
    protocorpus.ParseFromString(inputfile.read())

    for dd in protocorpus.doc_filenames:
      doc_num += 1
      if doc_num % 1000 == 0:
        print "Finished reading", doc_num, "documents."

      docfile = open("%s/%s" % (proto_corpus_dir, dd), 'rb')
      doc = Document()
      doc.ParseFromString(docfile.read())

      for jj in doc.sentences:
        for kk in jj.words:
          word = vocab_index_word[kk.token]
          # print word
          if word not in tfidf.keys():
            tfidf[word] = []
          
          tfidf[word].append(kk.tfidf)

    inputfile.close()

  tfidf_mean_output = open(output_dir + "/tfidf_mean.txt", 'w')
  tfidf_max_output = open(output_dir + "/tfidf_max.txt", 'w')
  tfidf_mid_output = open(output_dir + "/tfidf_mid.txt", 'w')
  
  for ww in tfidf.keys():
    tmp_max = 0
    tmp_mean = 0
    for val in tfidf[ww]:
      if val > tmp_max:
        tmp_max = val
      tmp_mean += val
    tmp_mean = tmp_mean / len(tfidf[ww])
    
    tmp = tfidf[ww].sort()
    index = int(len(tfidf[ww]) / 2)
    tmp_mid = tfidf[ww][index]
    
    tfidf_mean_output.write(ww + "\t" + str(tmp_mean) + "\n")
    tfidf_max_output.write(ww + "\t" + str(tmp_max) + "\n")
    tfidf_mid_output.write(ww + "\t" + str(tmp_mid) + "\n")

  tfidf_mean_output.close()
  tfidf_max_output.close()
  tfidf_mid_output.close()


def readStatistics(input_dir, topic_vocab, vocab_size):
  wordcount = defaultdict()
  cooccur = defaultdict()
  tfidf = defaultdict()
  wordcount_total = 0
  cooccur_total = 0

  # Smoothing
  for w1 in topic_vocab:
    tfidf[w1] = 0.
    wordcount[w1] = SMOOTH_FACTOR    
    cooccur[w1] = defaultdict()
    for w2 in topic_vocab:
      if w1 < w2:
        cooccur[w1][w2] = SMOOTH_FACTOR

  wordcount_total = SMOOTH_FACTOR * vocab_size
  cooccur_total = SMOOTH_FACTOR * vocab_size * (vocab_size - 1) / 2

  print "only smoothing: ", wordcount_total, cooccur_total

  # read wordcount
  filename = input_dir + "/wordcount.txt"
  infile = open(filename, 'r')
  for line in infile:
    line = line.strip()
    words = line.split('\t')
    if words[0] in wordcount.keys():
      wordcount[words[0]] += int(words[1])
    wordcount_total += int(words[1])
  infile.close()

  # read coccurance
  filename = input_dir + "/cooccurance.txt"
  infile = open(filename, 'r')
  for line in infile:
    line = line.strip()
    words = line.split('\t')
    if words[0] in cooccur.keys() and words[1] in cooccur.keys():
      cooccur[words[0]][words[1]] += int(words[2])
    cooccur_total += int(words[2])
  infile.close()

  print "all: ", wordcount_total, cooccur_total

  # read tfidf
  filename = input_dir + "/tfidf_max.txt"
  infile = open(filename, 'r')
  for line in infile:
    line = line.strip()
    words = line.split('\t')
    if words[0] in tfidf.keys():
      tfidf[words[0]] = float(words[1])
  infile.close()

  return cooccur, cooccur_total, tfidf, wordcount, wordcount_total


