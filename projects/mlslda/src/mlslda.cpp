/*
 * ldawn.cpp
 *
 * Copyright 2007, Jordan Boyd-Graber
 *
 * Class to implement a topic model with a ontological topic distribution over
 * words.
 */

#include <string>
#include <vector>
#include "topicmod/projects/mlslda/src/mlslda.h"

DEFINE_bool(regression_only, false,
            "Topics ignore response (regression on vanilla LDA)");
DEFINE_int(num_seed_docs, 0,
           "Initialize topics to reflect the response of the document");
DEFINE_double(min_variance, 1e-6,
              "Minimum variance value after regression");
DEFINE_double(variance, 0.25, "Initial variance");
DEFINE_bool(update_hyperparameters, false, "Update the variance");
DEFINE_bool(logistic, false, "Do logistic regression");
DEFINE_bool(ignore_test_counts, false, "Don't sample test documents");
DEFINE_int(num_test_iterations, 5,
           "If we don't sample test docs, estimate topics this long");

namespace topicmod_projects_mlslda {

void MlSldaState::train_topic_assignments(gsl_matrix* z) const {
  int M = corpus_->num_docs();
  int train_seen = 0;
  for (int ii = 0; ii < M; ++ii) {
    // We only use training docs for regression
    if (corpus_->doc(ii)->is_test()) continue;
    double scale = docs_[ii]->sum();

    // Empty documents can blow everything up if this is not here
    if (scale == 0) scale = 1.0;
    for (int kk = 0; kk < num_topics_; ++kk) {
      gsl_matrix_set(z, train_seen, kk, docs_[ii]->count(kk) / scale);
      BOOST_ASSERT(!isnan(gsl_matrix_get(z, train_seen, kk)));
    }
    ++train_seen;
  }
  assert(train_seen == corpus_->num_train());
}

void MlSldaState::InitializeResponse() {
  int all = 0, test = 0, train = 0;
  const gsl_vector* train_scores = static_cast
    <lib_corpora::ReviewCorpus*>(corpus_.get())->train_ratings();
  const gsl_vector* test_scores = static_cast
    <lib_corpora::ReviewCorpus*>(corpus_.get())->test_ratings();

  int num_docs = corpus_->num_docs();
  truth_.resize(num_docs);
  v_.resize(num_docs);
  effective_length_.resize(num_docs);

  for (all = 0; all < num_docs; ++all) {
    // Initialize the truth
    MlSeqDoc* doc = corpus_->seq_doc(all);
    if (doc->is_test()) {
      truth_[all] = gsl_vector_get(test_scores, test);
      ++test;
    } else {
      truth_[all] = gsl_vector_get(train_scores, train);
      ++train;
    }
    // If we're doing logistic regression, the response variable is only zero or
    // one; we transform based on whether the data are greater than the mean or
    // not.  (The corpus standardizes the data, so this is 0.0.)
    if (FLAGS_logistic) {
      if (truth_[all] >= 0)
        truth_[all] = 1.0;
      else
        truth_[all] = 0.0;
      v_[all] = 0.0;
    } else {
      v_[all] = truth_[all];
    }
  }
}

void MlSldaState::InitializeOther() {
  InitializeWordNet();
  InitializeRegression();
  // Unlike LDAWN, we do not initialize an answer cache, as we don't have
  // SemMlSeqDocs, we have review docs, which don't have semantic annotation
}

  // Mlslda needs to explicitly keep track of the length of documents; this is
  // where this gets initialized
void MlSldaState::InitializeLength() {
  int num_docs = corpus_->num_docs();

  for (int dd = 0; dd < num_docs; ++dd) {
    MlSeqDoc* doc = corpus_->seq_doc(dd);
    int lang = doc->language();

    // Initialize the number of words
    int num_words = 0;
    for (int jj = 0; jj < doc->num_tokens(); ++jj) {
      int term = (*doc)[jj];
      const topicmod_projects_ldawn::WordPaths word =
        wordnet_->word(lang, term);
      int num_paths = word.size();
      if (num_paths > 0) {
        ++num_words;
      } else {
        if (use_aux_topics())
          ++num_words;
      }
    }
    effective_length_[dd] = num_words;
    BOOST_ASSERT(num_words > 0);
  }
}

void MlSldaState::InitializeAssignments(bool random_init) {
  InitializeResponse();
  InitializeLength();

  LdawnState::InitializeAssignments(random_init);

  if (FLAGS_num_seed_docs > 0) {
    const gsl_vector* y = static_cast<lib_corpora::ReviewCorpus*>
      (corpus_.get())->train_ratings();
    boost::shared_ptr<gsl_permutation> sorted(gsl_permutation_alloc(y->size),
                                              gsl_permutation_free);
    boost::shared_ptr<gsl_permutation> rank(gsl_permutation_alloc(y->size),
                                            gsl_permutation_free);

    std::vector< std::vector<int> > num_seeds_used;
    num_seeds_used.resize(corpus_->num_languages());
    for (int ii = 0; ii < corpus_->num_languages(); ++ii) {
      num_seeds_used[ii].resize(num_topics_);
    }

    gsl_sort_vector_index(sorted.get(), y);
    gsl_permutation_inverse(rank.get(), sorted.get());

    // We add one for padding so we don't try to set a document to be equal to
    // the number of topics.
    double num_train = corpus_->num_train() + 1.0;
    int train_seen = 0;
    int num_docs = corpus_->num_docs();
    for (int dd = 0; dd < num_docs; ++dd) {
      MlSeqDoc* doc = corpus_->seq_doc(dd);
      int lang = doc->language();
      if (!corpus_->doc(dd)->is_test()) {
        // We don't assign to topic zero, so it can be stopwordy
        int val = (int) floor((num_topics_ - 1) *
                              rank->data[train_seen] / num_train) + 1;

        // Stop once we've used our limit of seed docs (too many leads to an
        // overfit initial state)
        if (num_seeds_used[lang][val] < FLAGS_num_seed_docs) {
          cout << "Initializing doc " << lang << " " << dd << " to " << val <<
            " score=" << truth_[dd] << endl;
          for (int jj = 0; jj < (int)topic_assignments_[dd].size(); ++jj) {
            int term = (*doc)[jj];
            const topicmod_projects_ldawn::WordPaths word =
              wordnet_->word(lang, term);
            int num_paths = word.size();
            if (num_paths > 0) {
              ChangePath(dd, jj, val, rand() % num_paths);
            } else {
              if (use_aux_topics())
                ChangeTopic(dd, jj, val);
            }
          }
          ++num_seeds_used[lang][val];
        }
        ++train_seen;
      }
    }
  }
}

void MlSldaState::ReadAssignments() {
  LdawnState::ReadAssignments();

  // Because nu starts as a zero vector, we need to optimize it so that we don't
  // loose all of our hard work
  OptimizeRegression();
}

void MlSldaState::InitializeRegression() {
  regression_space_.space_.reset
    (gsl_multifit_linear_alloc(corpus_->num_train(), num_topics_),
     gsl_multifit_linear_free);
  regression_space_.covariance_.reset(gsl_matrix_alloc(num_topics_,
                                                       num_topics_),
                                      gsl_matrix_free);
  regression_space_.x_.reset(gsl_matrix_alloc(corpus_->num_train(),
                                              num_topics_),
                            gsl_matrix_free);
  // This initialization of chi_quared_ makes it so the first estimator of
  // sigma_squared_ is 1.0
  regression_space_.chi_squared_ = corpus_->num_train() - num_topics_;
  nu_.reset(gsl_vector_alloc(num_topics_));

  double increment = 2.0 / (double)num_topics_;
  double current_nu = -1.0;
  for (int ii = 0; ii < num_topics_; ++ii) {
    gsl_vector_set(nu_.get(), ii, current_nu);
    current_nu += increment;
  }
  sigma_squared_ = FLAGS_variance;
}

double MlSldaState::OptimizeRegression() {
  assert((corpus_->num_train() - num_topics_) > 0);

  train_topic_assignments(regression_space_.x_.get());
  // Use gsl's implementation of OLS to find the new nu parameter
  const gsl_vector* y = static_cast<lib_corpora::ReviewCorpus*>
    (corpus_.get())->train_ratings();
  train_topic_assignments(regression_space_.x_.get());

  gsl_multifit_linear(regression_space_.x_.get(), y, nu_.get(),
                      regression_space_.covariance_.get(),
                      &regression_space_.chi_squared_,
                      regression_space_.space_.get());

  // Estimatee sigma squared from the sum of the squared residuals
  cout << "Did new regression with " << corpus_->num_train() << " documents "
    " and sum sq training error " << regression_space_.chi_squared_ <<
    " (" << regression_space_.chi_squared_ / (double)corpus_->num_train() <<
    ")" << endl;

  if (sigma_squared_ < FLAGS_min_variance) {
    sigma_squared_ = FLAGS_min_variance;
    cout << "WARNING! VARIANCE WENT TO ZERO.  SOMETHING CRAZY IS GOING ON.  ";
    cout << "WE'LL CARRY ON WITH VARIANCE=" << sigma_squared_ << endl;
  }

  // Now we update the predictions so that the numbers are correct
  UpdateAllPredictions();

  return regression_space_.chi_squared_;
}

void MlSldaState::UpdateDocPrediction(int doc) {
  if (FLAGS_logistic)
    v_[doc] = 0.0;
  else
    v_[doc] = truth_[doc];

  BOOST_ASSERT(doc < (int)topic_assignments_.size());
  double num_words = effective_length_[doc];
  int assigned = 0;
  for (int jj = 0; jj < (int)topic_assignments_[doc].size(); ++jj) {
    BOOST_ASSERT(num_words > 0.0);
    int assignment = topic_assignments_[doc][jj];
    if (assignment >= 0) {
      v_[doc] -= gsl_vector_get(nu_.get(), assignment) / num_words;
      assigned++;
    }
  }
  BOOST_ASSERT(assigned == effective_length_[doc]);
  BOOST_ASSERT(!isnan(v_[doc]));
}

void MlSldaState::UpdateAllPredictions() {
  int num_docs = corpus_->num_docs();
  for (int ii = 0; ii < num_docs; ++ii) {
    UpdateDocPrediction(ii);
  }
}

double MlSldaState::sigma_squared() const {
  return sigma_squared_;
}

const gsl_vector* MlSldaState::nu() const {
  return nu_.get();
}

vector<double> MlSlda::hyperparameters() {
  vector<double> p = Ldawn::hyperparameters();
  MlSldaState* state = static_cast<MlSldaState*>(state_.get());
  p.push_back(log(state->sigma_squared()));
  return p;
}

vector<string> MlSlda::hyperparameter_names() {
  vector<string> names = Ldawn::hyperparameter_names();
  names.push_back("sigma_squared");
  return names;
}

void MlSlda::SetHyperparameters(const vector<double>& hyperparameters) {
  Ldawn::SetHyperparameters(hyperparameters);
  MlSldaState* state = static_cast<MlSldaState*>(state_.get());
  state->set_sigma_squared(exp(hyperparameters[hyperparameters.size() - 1]));
}

double MlSlda::variance() const {
  MlSldaState* state = static_cast<MlSldaState*>(state_.get());
  return state->sigma_squared();
}

bool MlSlda::update_hyperparameters() const {
  return FLAGS_update_hyperparameters;
}

bool MlSlda::logistic() const {
  return FLAGS_logistic;
}

void MlSldaState::set_sigma_squared(double sigma_squared) {
  sigma_squared_ = sigma_squared;
  log_normal_normalizer_ = -0.5 * log(sqrt(2.0 * M_PI * sigma_squared_));
}

double MlSldaState::TopicDocProbability(int doc, int topic) {
  double val = 0;
  // If this is a test document, we marginalize out over the possible labels,
  // which means that the contribution of the label to the topic probability is
  // zero.
  if (!(corpus_->doc(doc)->is_test() || FLAGS_regression_only)) {
    // We add one because we decremented
    double num_words = effective_length_[doc];
    if (num_words > 0.0) {
      double nu = gsl_vector_get(nu_.get(), topic);
      val += TopicDocResponse(nu, num_words, v_[doc], truth_[doc],
                              sigma_squared_, FLAGS_logistic);
    }
  }
  BOOST_ASSERT(!isnan(val));
  val += LdawnState::TopicDocProbability(doc, topic);
  BOOST_ASSERT(!isnan(val));
  return val;
}

double MlSldaState::prediction(int doc) const {
  BOOST_ASSERT(doc < corpus_->num_docs());
  return truth_[doc] - v_[doc];
}

void MlSldaState::set_prediction(int doc, double prediction) {
  if (FLAGS_logistic)
    v_[doc] = -prediction;
  else
    v_[doc] = truth_[doc] - prediction;
}

void MlSldaState::UpdatePrediction(int doc, int index, int topic) {
  int current_topic = topic_assignments_[doc][index];

  // This is not exactly correct if the document starts uninitialized; it should
  // be right for subsequent iterations.
  double num_words = effective_length_[doc];
  if (current_topic >= 0) {
    v_[doc] += gsl_vector_get(nu_.get(), current_topic) / num_words;
  }

  if (topic >= 0) {
    v_[doc] -= gsl_vector_get(nu_.get(), topic) / num_words;
  }
  BOOST_ASSERT(!isnan(v_[doc]));
}

void MlSldaState::ChangePath(int doc, int index, int topic, int path) {
  UpdatePrediction(doc, index, topic);
  LdawnState::ChangePath(doc, index, topic, path);
}

void MlSldaState::ChangeTopic(int doc, int index, int topic) {
  UpdatePrediction(doc, index, topic);
  LdawnState::ChangeTopic(doc, index, topic);
}

double MlSldaState::WriteDisambiguation(int iteration) const {
  std::fstream outfile((output_ + ".pred").c_str(), ios::out);
  double total = 0.0;
  for (int ii = 0; ii < corpus_->num_docs(); ii++) {
    // If it's a test document, mark it
    lib_corpora::MlSeqDoc* doc = corpus_->seq_doc(ii);
    if (doc->is_test()) {
      outfile << "* ";
    }
    double error = v_[ii];
    total += error;
    double ans = truth_[ii];
    int lang = doc->language();
    // error = truth - pred -> pred = truth - error
    double pred = ans - error;
    outfile << ans << "\t" << pred << "\t" << pow(error, 2.0);
    outfile << "\t" << lang << "\t" << doc->id() << "\t[";
    double num_words = effective_length_[ii];
    for (int jj = 0; jj < num_topics_; ++jj) {
      if (jj > 0) outfile << ", ";
      double nu = gsl_vector_get(nu_.get(), jj);
      outfile << TopicDocResponse(nu, num_words, v_[ii], truth_[ii],
                                  sigma_squared_, FLAGS_logistic);
    }
    outfile << "]" << endl;
  }
  outfile.close();
  return total / (double)(corpus_->num_docs());
}

void MlSldaState::WriteTopicHeader(int topic, std::fstream* outfile) {
  (*outfile) << "-----------------------------------------" << endl;
  (*outfile) << "          Topic " << topic << endl;
  (*outfile) << "     Regression " << gsl_vector_get(nu_.get(), topic) << endl;
  (*outfile) << "-----------------------------------------" << endl;
}

MlSldaState::MlSldaState(int num_topics, string output,
                         string wn_location, bool logistic)
  : LdawnState(num_topics, output, wn_location) {
  logistic_ = logistic;
}

MlSlda::MlSlda(double alpha_sum, double alpha_zero, double lambda,
               int num_topics, string wn_location)
  : Ldawn(alpha_sum, alpha_zero, lambda, num_topics, wn_location) {}

void MlSlda::InitializeState(const string& output_location, bool random_init) {
  cout << "Creating new MLSLDA State" << endl;
  state_.reset(new MlSldaState(num_topics_, output_location,
                               wordnet_location_, FLAGS_logistic));
  state_->Initialize(corpus_, random_init, alpha_sum_, alpha_zero_, lambda_);
}

bool MlSlda::IsDocSkippable(MlSeqDoc* doc) {
  // Only skip documents if this document is a test doc and we're skipping test
  // documents
  return doc->is_test() && FLAGS_ignore_test_counts;
}

double MlSlda::TestLikelihoodIncrement() {
  double val = 0.0;
  MlSldaState* state = static_cast<MlSldaState*>(state_.get());
  for (int dd = 0; dd < corpus_->num_docs(); ++dd) {
    lib_corpora::MlSeqDoc* doc = corpus_->seq_doc(dd);

    if (doc->is_test()) {
      if (FLAGS_ignore_test_counts) {
        // Sample the words
        int num_words = doc->num_tokens();

        for (int ww = 0; ww < num_words; ++ww)
          val += LogProbLeftToRightIncrement(dd, ww);

        for (int ii = 0; ii < FLAGS_num_test_iterations; ++ii) {
          for (int ww = 0; ww < num_words; ++ww) SampleWord(dd, ww);
        }
      }

      val += state->ComputeResponseProbability(dd);

      if (FLAGS_ignore_test_counts)
        ResetTest(dd);
    }
  }
  return val;
}

void MlSldaState::WriteNu() {
  fstream outfile((output_ + ".regression").c_str(), ios::out);
  for (int ii = 0; ii < num_topics_; ++ii)
    outfile << gsl_vector_get(nu_.get(), ii) << endl;
  outfile.close();
}

void MlSlda::WriteReport() {
  Ldawn::WriteReport();
  static_cast<MlSldaState*>(state_.get())->WriteNu();
}

void MlSlda::SampleCorpusVariables(int iter, double step, int samples,
                                    std::ostream* param_file) {
  // This if is here because we can't optimize regression during the first
  // iteration.  This behavior overloads the samples arguments and probably
  // isn't ideal.  TODO(jbg): make this cleaner.
  if (samples > 0)
    static_cast<MlSldaState*>(state_.get())->OptimizeRegression();
  if (FLAGS_update_hyperparameters)
    Ldawn::SampleCorpusVariables(iter, step, samples, param_file);
  else
    Ldawn::SampleCorpusVariables(iter, 0.0, 0, param_file);
}

double MlSlda::WriteLhoodUpdate(std::ostream* lhood_file, int iteration,
                                double elapsed) {
  double train = static_cast<MlSldaState*>(state_.get())->ComputeError(true);
  double test = static_cast<MlSldaState*>(state_.get())->ComputeError(false);
  double lhood = Ldawn::WriteLhoodUpdate(lhood_file, iteration, elapsed);
  (*lhood_file) << "\t" << train << "\t" << test;
  cout << "Test error: " << test << " train error: " << train << endl;
  return lhood;
}

bool MlSlda::regression_only() {
  return FLAGS_regression_only;
}

double MlSldaState::ComputeResponseProbability(int doc) {
  // Should compute the likelihood of the observed response given current
  // assignments
  double p = -(v_[doc] * v_[doc]) / (2.0 * sigma_squared_);
  p += log_normal_normalizer_;
  return p;
}

double MlSldaState::ComputeError(bool use_training) {
  double sum = 0.0;
  int M = corpus_->num_docs(), train = 0, test = 0;

  // Note that v_ stores the negative prediction, so to subtract we add the
  // truth to it
  for (int all = 0; all < M; ++all) {
    lib_corpora::MlSeqDoc* doc = corpus_->seq_doc(all);
    double error = v_[all] * v_[all];
    if (FLAGS_logistic)
      error = (v_[all] - 0.5) * (truth_[all] - 0.5) >= 0.0 ? 0.0 : 1.0;
    if (doc->is_test()) {
      if (!use_training) sum += error;
      ++test;
    } else {
      if (use_training) sum += error;
      ++train;
    }
    BOOST_ASSERT(!isnan(error));
    BOOST_ASSERT(!isnan(sum));
  }

  if (use_training && train > 0) sum /= (double)train;
  if (!use_training && test > 0) sum /= (double)test;
  double test_variance = static_cast
    <lib_corpora::ReviewCorpus*>(corpus_.get())->test_variance();
  // if (!use_training && test > 0) sum /= test_variance;

  return sum;
}

double MlSlda::Likelihood() {
  MlSldaState* s = static_cast<MlSldaState*>(state_.get());
  double val = 0;
  val += state_->TopicLikelihood();
  val += state_->DocLikelihood();
  for (int dd = 0; dd < corpus_->num_docs(); ++dd) {
    val += s->ComputeResponseProbability(dd);
  }
  return val;
}

void MlSlda::ResetTest(int doc) {
  MlSldaState* s = static_cast<MlSldaState*>(state_.get());
  s->UpdateDocPrediction(doc);
  double prediction = s->prediction(doc);
  Ldawn::ResetTest(doc);
  // We need to set the prediction again because changing the state will zero it
  // out.
  s->set_prediction(doc, prediction);
  prediction = s->prediction(doc);
}

}  // end namespace project_ldawn
