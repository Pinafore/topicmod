/*
 * Copyright 2007, Jordan Boyd-Graber
 */

#include "topicmod/lib/lda/sampler_main.h"
#include "topicmod/projects/ldawn/src/ldawn.h"
#include "topicmod/lib/util/flags.h"

DEFINE_string(wordnet_location, "wn/wordnet.wn.0,wn/wordnet.wn.1,"
              "wn/wordnet.wn.2,wn/wordnet.wn.3,wn/wordnet.wn.4,"
              "wn/wordnet.wn.5,wn/wordnet.wn.6,wn/wordnet.wn.7",
              "Where we read the wordnet from");

using lib_corpora::SemMlSeqCorpus;
using lib_corpora::SemanticCorpusReader;

int main(int argc, char **argv) {
  InitFlags(argc, argv);

  scoped_ptr<CorpusReader>
    reader(new SemanticCorpusReader(FLAGS_lemmatize, FLAGS_bigrams,
                                    FLAGS_remove_stop, FLAGS_min_words,
                                    FLAGS_train_limit, FLAGS_test_limit));

  SemMlSeqCorpus* corpus = LoadAndRelabelCorpus<SemMlSeqCorpus>(reader.get());

  boost::scoped_ptr<lib_lda::Sampler>
    s(new topicmod_projects_ldawn::Ldawn(FLAGS_alpha, FLAGS_alpha_zero_scale,
                                         FLAGS_lambda, FLAGS_num_topics,
                                         FLAGS_wordnet_location));
  s->Initialize(corpus);
  s->RunSampler(FLAGS_num_iterations);
}


