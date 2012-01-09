from collections import defaultdict
from glob import glob
from math import log
from topicmod.util import flags
from topicmod.ling.snowball_wrapper import Snowball
import nltk
from nltk.tokenize import wordpunct_tokenize
from nltk.tokenize import PunktWordTokenizer
from PMI_statistics import get_tfidf


class corpusParser():


  def __init__(self, lang, vocab_dir, corpus_dir, window_size, output_dir):
    self._lang = 0
    self._vocab_dir = vocab_dir
    self._corpus_dir = corpus_dir
    self._window_size = window_size
    self._output_dir = output_dir
    self._stemmer = Snowball()
    self._sent_tokenizer = nltk.data.load('tokenizers/punkt/english.pickle')
    self._word_tokenizer = PunktWordTokenizer()
    self._cooccur = defaultdict()
    self._wordcount = defaultdict()
    self._vocab = set()
    self._doc_num = 0
    #self._vocab_word_index = defaultdict()
    #self._vocab_index_word = defaultdict()


  def loadVocab(self):
    vocabfile = open(self._vocab_dir, 'r')
    vocab_word_index = defaultdict()
    vocab_index_word = []
    #index = -1
    for line in vocabfile:
      line = line.strip()
      words = line.split('\t')
      word = words[1]
      self._vocab.add(word)
      #index += 1
      #self._vocab_word_index[word] = index
      #self._vocab_index_word.append(word)

    vocabfile.close()

    # Initialize wordcount and cooccur
    for word in self._vocab:
      self._wordcount[word] = 0
      self._cooccur[word] = defaultdict()
      for word2 in self._vocab:
        if word2 > word:
          self._cooccur[word][word2] = 0
    

  def parseDoc(self, doc_raw):
    tokens = []
    for sent in self._sent_tokenizer.tokenize(doc_raw):
      for token in self._word_tokenizer.tokenize(sent):
        tokens.append(self._stemmer(self._lang, token))

    tokens_len = len(tokens)
    for index1 in range(0, tokens_len):
      w1 = tokens[index1]
      if w1 in self._vocab:
        self._wordcount[w1] += 1

        if self._window_size == -1:
          index_end = tokens_len
        else:
          index_end = min(tokens_len, index1 + self._window_size)

        for index2 in range(index1 + 1, index_end):
          w2 = tokens[index2]
          if w2 in self._vocab:
            if w1 < w2:
              self._cooccur[w1][w2] += 1
            elif w1 > w2:
              self._cooccur[w2][w1] += 1


  def parseCorpus20news(self):

    print "Loading vocab"
    self.loadVocab()
    
    doc_count = 0

    print "Parsing corpus"
    data_folders = [self._corpus_dir + "/train", self._corpus_dir + "/test"]
    print data_folders
    for data_folder in data_folders:
      for folder in glob("%s/*^tgz" % data_folder):
        for ff in glob("%s/*" % folder):
          doc_count += 1
          infile = open(ff, 'r')
          doc_raw = ""
          for line in infile:
            line = line.strip().lower()
            doc_raw += " " + line
          self.parseDoc(doc_raw)
          infile.close()
          if doc_count % 1000 == 0:
            print "Finish parsing", doc_count, "documents!"

    self._doc_num = doc_count
    print "Total number of docunments: ", doc_count
    print "writing results!"
    self.writeResult()


  def parseCorpusNyt(self):

    print "Loading vocab"
    self.loadVocab()
    
    doc_count = 0

    print "Parsing corpus"

    years = ["1987", "1988", "1989", "1990", "1991", "1992", "1993", "1994", "1995", "1996"]
    print data_folders

    for year in years:
      folder_year = self._corpus_dir + "/" + year
      for month in glob("%s/[0-9][0-9]" % folder_year):
        for day in glob("%s" % month):
          for ff in glob("%s/*" % day):
            doc_count += 1
            infile = open(ff, 'r')
            doc_raw = ""
            for line in infile:
              line = line.strip().lower()
              doc_raw += " " + line
            self.parseDoc(doc_raw)
            infile.close()
            if doc_count % 1000 == 0:
              print "Finish parsing", doc_count, "documents!"

    self._doc_num = doc_count
    print "Total number of docunments: ", doc_count
    print "writing results!"
    self.writeResult()


  def parseCorpusWiki(self):
    print "Loading vocab"
    self.loadVocab()

    print "Parsing corpus"
    doc_count = 0
    file_count = 0
    for folder in glob("%s/*" % self._corpus_dir):
      for ff in glob("%s/*" % folder):
        infile = open(ff, 'r')
        file_count += 1
        if file_count % 100 == 0:
          print "Finish parsing", file_count, "files or ", doc_count, "documents!"

        for line in infile:
          line = line.strip().lower()

          if line.startswith("<doc"):
            doc_count += 1
            doc_flag = True
            doc_raw = ""
          elif line.startswith("</doc>"):
            doc_flag = False
            ### processing doc
            self.parseDoc(doc_raw)
          else:
            assert doc_flag == True
            doc_raw += " " + line
        infile.close()

    self._doc_num = doc_count
    print "Total number of docunments: ", doc_count
    self.writeResult()


  def writeResult(self):
    # write wordcount
    outputfile = self._output_dir + "/wordcount.txt"
    outfile = open(outputfile, 'w')
    for word in self._wordcount.keys():
      tmp = word + "\t" + str(self._wordcount[word]) + "\n"
      outfile.write(tmp)
    outfile.close()

    # write coccurance:
    outputfile = self._output_dir + "/cooccurance.txt"
    outfile = open(outputfile, 'w')
    for w1 in self._cooccur.keys():
      for w2 in self._cooccur[w1].keys():
        if self._cooccur[w1][w2] != 0:
          tmp = w1 + "\t" + w2 + "\t" + str(self._cooccur[w1][w2]) + "\n"
          outfile.write(tmp)
    outfile.close()


flags.define_string("corpus", None, "Where we find the input corpora")
flags.define_string("proto_corpus", None, "Where we find the input proto corpora")
flags.define_string("vocab", "", "The model files folder of topic models")
flags.define_int("window_size", 10, "Size of window for computing coocurrance")
flags.define_string("output", "PMI_stat/20_news", "PMI stat output filename")
flags.define_int("option", "2", "0: 20_news; 1: wikipedia")

if __name__ == "__main__":
  flags.InitFlags()
  # {0: 'english', 1: 'german'}
  lang = 0
  cp = corpusParser(lang, flags.vocab, flags.corpus, flags.window_size, flags.output)
  if flags.option == 0:
    cp.parseCorpus20news()
    get_tfidf(flags.proto_corpus, flags.vocab, flags.output)
  elif flags.option == 1:
    cp.parseCorpusWiki()
    get_tfidf(flags.proto_corpus, flags.vocab, flags.output)
  elif flags.option == 2:
    cp.parseCorpusNyt()
    get_tfidf(flags.proto_corpus, flags.vocab, flags.output)

