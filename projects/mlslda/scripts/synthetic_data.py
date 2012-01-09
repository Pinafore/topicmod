
from random import random
from collections import defaultdict
import os

import numpy
from numpy import zeros
from numpy.random.mtrand import dirichlet
from numpy.random import multinomial
from numpy.random import normal
from math import isnan, isinf

from topicmod.util import flags
from topicmod.corpora.proto.corpus_pb2 import *

flags.define_int("num_docs", 500, "Number of documents")
flags.define_int("num_topics", 5, "Number of topics")
flags.define_int("doc_length", 5, "Length of every document")
flags.define_int("num_langs", 2, "Number of languages")
flags.define_float("variance", 0.5, "Variance of distribution")
flags.define_float("gamma", 1.0, "Vocabulary hyperparameter")
flags.define_float("alpha", 0.1, "Document topic hyperparameter")
flags.define_string("output_base", "data/synthetic", "Where we write the data")
flags.define_string("doc_proportion", "synthetic.theta", "Where we write doc thetas")
flags.define_int("num_groups", 2, "Number of splits")
flags.define_string("vocab_output", "vocab/synthetic.voc", "Where we write vocab")
flags.define_int("topic_output_size", 15, "Number of words to display when we output topics")

ml_vocab = [{0: ["dog", "cat", "moose", "butterfly"], 
 1: ["hund", "katze", "spinner", "pferd", "maultier", "kuh"], 
 2:["toro", "mariposa", "gato", "vaca", "donkey", "burro", "caballo", "mosquito", "arana", "pavo"]},
{0: ["monday", "tuesday", "thursday", "friday", "saturday"],
 1: ["montag", "dienstag", "mitwoch", "donnerstag", "freitag", "samstag", "sontag"], 
 2: ["lunes", "martes", "miercoles", "jueves", "viernes", "sabado", "domingo"]},
{0: ["mop", "broom", "bucket", "rake"],
 1: ["mopp", "besen", "eimer", "moebelpolitur"],
 2: ["trapeador", "escoba", "cubeta", "rastrillo"]},
{0: ["north", "east", "south", "west"],
 1: ["nord", "ost", "sud", "west"],
 2: ["norte", "este", "usr", "oeste", "occidente", "oriente"]},
{0: ["one", "two", "three", "four", "five", "six"],
 1: ["eins", "zwo", "drei", "zwei", "vier", "funf"],
 2: ["uno", "dos", "tres", "quatrro", "cinco"]}]

vocab_lookup = defaultdict(dict)
for ii in ml_vocab:
  for ll in ii:
    for jj in ii[ll]:
      vocab_lookup[ll][jj] = len(vocab_lookup[ll])

def DirichletDraw(alpha):
  t = dirichlet(alpha)
  while isnan(t[0]) or isinf(t[0]):
    t = dirichlet(alpha)
  return t

def write_proto(filename, proto):
  f = open(filename, "wb")
  f.write(proto.SerializeToString())
  f.close()

class Doc:
  def __init__(self):
    self.words = []
    self.response = 0.0
    self.lang = 0
    self.id = ""
    self.num = -1
    self.theta = None

  def Write(self, filename, internal_id):
    doc = Document()
    doc.language = self.lang
    doc.id = internal_id
    doc.rating = self.response
    doc.author = 0
    s = doc.sentences.add()
    for ii in self.words:
       w = s.words.add()
       w.token = vocab_lookup[self.lang][ii]
       w.lemma = vocab_lookup[self.lang][ii]
    self.num = internal_id
    write_proto(filename, doc)

def WriteCorpus(docs, vocab, filename, vocab_output, doc_output):
  o = open(vocab_output, 'w')
  for lang, term in vocab:
    o.write("%i\t%s\n" % (lang, term))
  o.close()

  o = open(doc_output, 'w')

  for div in xrange(flags.num_groups):
    c = Corpus()
    for ii in docs:
      assert not docs[ii].num == -1
      if docs[ii].num % flags.num_groups == div:
        c.doc_filenames.append(docs[ii].id)
        o.write("%i %s %f\n" % (docs[ii].num, str(docs[ii].theta), docs[ii].response))

    a = c.authors.terms.add()
    a.id = 0
    a.original = "Default author"

    for ii in xrange(flags.num_langs):
      v = c.tokens.add()
      v.language = ii
      for jj in [x[1] for x in vocab if x[0] == ii]:
        term = v.terms.add()
        term.frequency = vocab[(ii, jj)]
        term.original = jj
        term.id = vocab_lookup[ii][jj]

    write_proto("%s-%i.index" % (filename, div), c)

if __name__ == "__main__":
  flags.InitFlags()

  beta = {}
  eta = zeros(flags.num_topics)
  vocab_total = defaultdict(int)
  for ii in xrange(flags.num_topics):
    eta[ii] = ii + random() * float(ii)
    for ll in xrange(flags.num_langs):
      print ml_vocab[ii]
      print ml_vocab[ii][ll]
      gamma = [flags.gamma] * len(ml_vocab[ii][ll])
      beta[(ll, ii)] = dirichlet(gamma)
      print "BETA", (ll, ii), beta[(ll, ii)]

  theta = {}
  alpha = [flags.alpha / float(flags.num_topics)] * flags.num_topics

  docs = defaultdict(Doc)

  print "Variance", flags.variance, flags.variance > 0

  for ll in xrange(flags.num_langs):
    for ii in [(ll, x) for x in xrange(flags.num_docs)]:
      z_bar = zeros(flags.num_topics)
      theta[ii] = DirichletDraw(alpha)
      docs[ii].lang = ll
      docs[ii].theta = theta[ii]
      docs[ii].id = "synth/%i_%i" % ii
      for jj in xrange(flags.doc_length):
        z = multinomial(1, theta[ii]).argmax()
        w = multinomial(1, beta[(ll, z)]).argmax()
        w = ml_vocab[z][ll][w]
        docs[ii].words.append(w)
        vocab_total[(ll, w)] += 1
        z_bar[z] += 1
      z_bar /= sum(z_bar)

      if flags.variance > 0:
        docs[ii].response = normal(sum(z_bar * eta), flags.variance)
      else:
        docs[ii].response = sum(z_bar * eta)
      print z_bar, docs[ii].response

  try:
      os.mkdir("%s/synth" % flags.output_base)
  except OSError:
      print("Dir already exists")

  doc_num = 0
  for ii in docs:
    docs[ii].Write("%s/%s" % (flags.output_base, docs[ii].id), doc_num)
    doc_num += 1

  WriteCorpus(docs, vocab_total, "%s/synth" % flags.output_base, flags.vocab_output, "%s/%s" % (flags.output_base, flags.doc_proportion))
