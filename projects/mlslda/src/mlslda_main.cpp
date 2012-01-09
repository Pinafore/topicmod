/*
 * Copyright 2007, Jordan Boyd-Graber
 */

#include <string>
#include <set>
#include <vector>

#include "topicmod/lib/lda/sampler_main.h"
#include "topicmod/projects/ldawn/src/ldawn.h"
#include "topicmod/lib/util/flags.h"
#include "topicmod/lib/util/strings.h"
#include "topicmod/projects/mlslda/src/mlslda.h"

DEFINE_string(wordnet_location, "wn/wordnet.wn.0,wn/wordnet.wn.1,"
              "wn/wordnet.wn.2,wn/wordnet.wn.3,wn/wordnet.wn.4,"
              "wn/wordnet.wn.5,wn/wordnet.wn.6,wn/wordnet.wn.7",
              "Where we read the wordnet from");
DEFINE_list(allowed_ratings, "", "The ratings we allow");
DEFINE_int(max_train_ratings, -1,
           "The maximum number of identical train ratings we can have");
DEFINE_int(max_test_ratings, -1,
           "The maximum number of identical test ratings we can have");

using lib_corpora::ReviewCorpus;
using lib_corpora::ReviewReader;
using topicmod_projects_mlslda::MlSlda;

void WriteSummary(const string& output, int seed, double sigma,
                  bool aux_topics, bool regression_only, bool update_hyperparameters,
                  double variance, bool logistic) {
  fstream outfile((output + ".about").c_str(), ios::out);
  outfile << "topics " << FLAGS_num_topics << endl;
  outfile << "corpus " << FLAGS_corpus_location << endl;
  outfile << "test ";
  for (int ii = 0; ii < (int)FLAGS_test_section.size(); ++ii)
    outfile << FLAGS_test_section[ii] << " ";
  outfile << endl;

  outfile << "train ";
  for (int ii = 0; ii < (int)FLAGS_train_section.size(); ++ii)
    outfile << FLAGS_train_section[ii] << " ";
  outfile << endl;

  outfile << "wordnet " << FLAGS_wordnet_location << endl;
  outfile << "alpha " << FLAGS_alpha << endl;
  outfile << "alpha_0 " << FLAGS_alpha_zero_scale << endl;
  outfile << "lambda " << FLAGS_lambda << endl;
  outfile << "rand_seed " << seed << endl;
  outfile << "sigma squared " << sigma << endl;
  outfile << "response ";
  if (regression_only)
    outfile << "lda" << endl;
  else
    outfile << "slda" << endl;
  outfile << "aux_topics " << aux_topics << endl;

  outfile << "regression ";
  if (logistic)
    outfile << "logistic" << endl;
  else
    outfile << "normal" << endl;

  outfile << "update_hyperparameters ";
  if (update_hyperparameters)
    outfile << "true" << endl;
  else
    outfile << "false" << endl;
  outfile << "initial_variance " << variance << endl;
}

int main(int argc, char **argv) {
  InitFlags(argc, argv);

  // Get the allowed ratings
  std::set<double> valid_ratings;
  for (uint ii = 0; ii < FLAGS_allowed_ratings.size(); ++ii) {
    int rating =
      lib_util::ParseLeadingDoubleValue(FLAGS_allowed_ratings[ii].c_str());
    valid_ratings.insert(rating);
  }

  scoped_ptr<CorpusReader>
    reader(new ReviewReader(FLAGS_lemmatize, FLAGS_bigrams,
                            FLAGS_remove_stop, FLAGS_min_words,
                            FLAGS_train_limit, FLAGS_test_limit,
                            FLAGS_max_train_ratings, FLAGS_max_test_ratings,
                            valid_ratings));

  ReviewCorpus* corpus = LoadAndRelabelCorpus<ReviewCorpus>(reader.get());

  boost::scoped_ptr<topicmod_projects_mlslda::MlSlda>
    s(new MlSlda(FLAGS_alpha, FLAGS_alpha_zero_scale,
                 FLAGS_lambda, FLAGS_num_topics,
                 FLAGS_wordnet_location));
  s->Initialize(corpus);

  WriteSummary(s->output_path(), s->rand_seed(), s->variance(),
               s->use_aux_topics(), s->regression_only(), s->update_hyperparameters(),
               s->variance(), s->logistic());

  s->RunSampler(FLAGS_num_iterations);
}
