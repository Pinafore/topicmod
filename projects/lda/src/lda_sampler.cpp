/*
 * Copyright 2009, Jordan Boyd-Graber
 */

#include "../../../lib/lda/sampler_main.h"

int main(int argc, char **argv) {
  InitFlags(argc, argv);
  scoped_ptr<SemanticCorpusReader>
    reader(new SemanticCorpusReader(FLAGS_lemmatize, FLAGS_bigrams,
                                    FLAGS_remove_stop, FLAGS_min_words,
                                    FLAGS_train_limit, FLAGS_test_limit));

  lib_corpora::MlSeqCorpus* corpus =
    LoadAndRelabelCorpus<MlSeqCorpus>(reader.get());

  boost::scoped_ptr<lib_lda::Sampler>
    s(new lib_lda::Sampler(FLAGS_alpha, FLAGS_alpha_zero_scale,
                           FLAGS_lambda, FLAGS_num_topics));
  s->Initialize(corpus);
  s->RunSampler(FLAGS_num_iterations);
}
