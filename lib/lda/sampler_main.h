/*
 * sampler_main.h
 *
 * Copyright 2009, Jordan Boyd-Graber
 *
 * File to be included by the driver for a sampler.  Defines many different
 * command line options, also provides code to initialize a corpus.
 *
 */

#ifndef TOPICMOD_LIB_LDA_SAMPLER_MAIN_H_
#define TOPICMOD_LIB_LDA_SAMPLER_MAIN_H_

#include <boost/scoped_ptr.hpp>

#include <string>
#include <vector>

#include "topicmod/lib/util/flags.h"
#include "topicmod/lib/corpora/corpus.h"
#include "topicmod/lib/corpora/corpus_reader.h"

#include "topicmod/lib/lda/sampler.h"

using boost::scoped_ptr;
using lib_corpora::MlSeqCorpus;
using lib_corpora::CorpusReader;
using lib_corpora::SemanticCorpusReader;
using lib_corpora::ReviewReader;

DEFINE_double(alpha, 0.1, "Topic hyperparameter");
DEFINE_double(alpha_zero_scale, 1.0,
              "How much we scale topic zero (Hanna's stopword trick)");
DEFINE_double(lambda, 0.01, "Vocab hyperparameter");
DEFINE_int(num_topics, 5, "Number of topics");
DEFINE_bool(remove_stop, true, "Do we remove stop words");
DEFINE_bool(lemmatize, false, "Do we lemmatize words");
DEFINE_bool(bigrams, false, "Do we use bigrams");
DEFINE_int(min_words, 20, "Number of words minimum");
DEFINE_int(test_limit, -1, "Number of test documents to load");
DEFINE_int(train_limit, -1, "Number of train documents to load");
DEFINE_int(num_iterations, 10000, "Number of iterations we run the sampler");
DEFINE_string(vocab_file, "vocab/semcor.voc", "The vocab file");
DEFINE_string(corpus_location, "../../data/semcor/numeric/",
              "Where the corpus lives");
DEFINE_list(test_section, "", "The files we test on");
DEFINE_list(train_section, "semcor_english_0.index", "The files we train on");
DEFINE_string(pos_filter, "", "A list of parts of speech we explicitly allow");

// Template is the corpus type
template <class T> T* LoadAndRelabelCorpus(CorpusReader* reader) {
  lib_corpora::MlVocab* vocab = new lib_corpora::MlVocab();
  lib_corpora::MlVocab pos;

  // Read the vocab
  assert(FLAGS_vocab_file != "");
  lib_corpora::ReadVocab(FLAGS_vocab_file, vocab);

  // Read the parts of speech
  if (FLAGS_pos_filter != "")
    lib_corpora::ReadVocab(FLAGS_pos_filter, &pos);

  // State class will take ownership of the corpus
  T* corpus = new T(vocab);
  for (int ii = 0; ii < (int)FLAGS_train_section.size(); ++ii)
    reader->ReadProtobuf(FLAGS_corpus_location, FLAGS_train_section[ii], false,
                         *vocab, pos, corpus);

  for (int ii = 0; ii < (int)FLAGS_test_section.size(); ++ii)
    reader->ReadProtobuf(FLAGS_corpus_location, FLAGS_test_section[ii], true,
                         *vocab, pos, corpus);

  return corpus;
}

#endif  // TOPICMOD_LIB_LDA_SAMPLER_MAIN_H_
