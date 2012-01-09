/*
 * Copyright 2009 Jordan Boyd-Graber
 * 
 * Basic gibbs sampler for LDA-based models
 */
#include <string>
#include <vector>

#include "topicmod/lib/lda/sampler.h"

DEFINE_int(global_sample_reps, 5, "How many times we do slice "
           "sampling for hyperparameters");
DEFINE_double(global_sample_step, 1.0, "Step size for slice sampling");
DEFINE_int(num_threads, 1, "Number of threads used by OpenMP");
DEFINE_string(output_location, "output/model", "Where results are written.");
DEFINE_int(save_delay, 25, "How many iterations between saving state.");
DEFINE_int(global_sample_burnin, 100, "How many iterations we wait before"
           " sampling hyperparameters");
DEFINE_int(global_sample_delay, 20,
           "Iterations between sampling corpus level parameters");
DEFINE_bool(random_init, false, "Uniformly sample initial topics.");
DEFINE_bool(resume, false, "Load from a previous state.");
DEFINE_bool(verbose_assignments, false,
            "Write verbose assignments as protocol buffer");
DEFINE_bool(mallet_assignments, false,
            "Also write mallet assignments");
DEFINE_int(rand,
           (int)time(NULL),
           "Random number generator seed");
DEFINE_int(num_topic_terms,
           15,
           "How many words do we print out for each topic.");

namespace lib_lda {

Sampler::Sampler(double alpha_sum,
                 double alpha_zero,
                 double lambda,
                 int num_topics) {
  alpha_sum_          = alpha_sum;
  alpha_zero_         = alpha_zero;
  lambda_             = lambda;
  num_topics_         = num_topics;
  iteration_          = 0;
  num_threads_        = FLAGS_num_threads;
  hyperparameter_file_ = false;
  params_not_default_ = false;
  if (alpha_sum_ != 0.1 || lambda_ != 0.01)
    params_not_default_ = true;
}

void Sampler::Initialize(lib_corpora::MlSeqCorpus* corpus) {
  corpus_ = corpus;
  InitializeState(FLAGS_output_location, FLAGS_random_init);
  InitializeTemp();
}

const string& Sampler::output_path() {
  return state_->output_path();
}

int Sampler::rand_seed() {
  return FLAGS_rand;
}

void Sampler::InitializeState(const string& output_location, bool random_init) {
  cout << "Creating new LDA State" << endl;
  state_.reset(new State(num_topics_, output_location));
  state_->Initialize(corpus_, random_init, alpha_sum_, alpha_zero_, lambda_);
}

void Sampler::InitializeTemp() {
  // Create an array to hold the things probabilities that we'll
  // sample from (this should be as large as we're going sample)
  temp_probs_.resize(num_topics_);
}

double Sampler::TopicProbability(int doc,
                                 int index,
                                 int topic) {
  size_t term = (*corpus_->seq_doc(doc))[index];
  double doc_val = state_->TopicDocProbability(doc, topic);
  double voc_val = state_->TopicVocProbability(topic, term);

  return doc_val + voc_val;
}

int Sampler::SampleWord(int doc, int index) {
  PrepareWord(doc, index);
  return SampleWordTopic(doc, index);
}

void Sampler::PrepareWord(int doc, int index) {}

int Sampler::SampleWordTopic(int doc, int index) {
  // First, decrement the count if it's assigned
  state_->ChangeTopic(doc, index, -1);

  int k = num_topics_;

  #pragma omp parallel num_threads(num_threads_)
  #pragma omp for schedule (static)
  for (int ii = 0; ii < k; ++ii) {
    double p = TopicProbability(doc, index, ii);
    temp_probs_[ii] = p;
    BOOST_ASSERT(!isnan(p));
    BOOST_ASSERT(ii < (int)temp_probs_.size());
  }

  // Now we choose a topic
  int new_topic = log_vector_sample(temp_probs_, k);
  state_->ChangeTopic(doc, index, new_topic);

  BOOST_ASSERT(state_->topic_assignment(doc, index) >= 0);
  BOOST_ASSERT(state_->topic_assignment(doc, index) == new_topic);
  BOOST_ASSERT(new_topic < k);
  return new_topic;
}

void Sampler::WriteReport() {
  state_->WriteState(FLAGS_verbose_assignments, FLAGS_mallet_assignments);
  state_->WriteHyperparameters(hyperparameters());
  state_->WriteTopics(FLAGS_num_topic_terms);
  state_->WriteDocumentTotals();
}

bool Sampler::IsDocSkippable(lib_corpora::MlSeqDoc* doc) {
  return doc->is_test();
}

void Sampler::RunSampler(int num_iterations) {
  std::ofstream lhood_file;
  std::ofstream param_file;

  int initial_iteration = 0;
  if (FLAGS_resume) {
    initial_iteration = state_->ReadState(FLAGS_save_delay) + 1;

    vector<double> params_file =
                      state_->ReadHyperparameters(num_hyperparameters());
    SetHyperparameters(params_file);

    // sometimes, we use resume, but the parameters setting are different.
    // so if the params are not default, which means the params are updated,
    // we will just load the params from the CMD line,
    // else, just use the resume params.
    if (params_not_default_) {
      vector<double> params_cmdl = hyperparameters();
      BOOST_ASSERT(params_file.size() == params_cmdl.size());
      for (int ii = 0; ii < params_file.size(); ++ii) {
        if ((int) params_file[ii] * 1.0e10 !=
                                  (int) params_cmdl[ii] * 1.0e10) {
          cout << "Warning: parameters in CMD are not the same "
               << "as in params file." << endl;
          break;
        }
      }
    } else {
      SetHyperparameters(params_file);
    }

    lhood_file.open((FLAGS_output_location + ".lhood").c_str(), ios::app);
    param_file.open((FLAGS_output_location + ".param_hist").c_str(), ios::app);

  } else {
    lhood_file.open((FLAGS_output_location + ".lhood").c_str(), ios::out);
    param_file.open((FLAGS_output_location + ".param_hist").c_str(), ios::out);

    vector<string> param_names = hyperparameter_names();
    for (int ii = 0; ii < (int)param_names.size(); ++ii)
      param_file << param_names[ii] << "\t";
    param_file << endl;
  }

  double lhood = -1e10;

  // Write out the initial hyperparameters
  SampleCorpusVariables(initial_iteration, 0.0, 0, &param_file);
  // if ((initial_iteration / FLAGS_save_delay + 1) *
  //    FLAGS_save_delay > num_iterations) {
  if ((num_iterations - initial_iteration + 1) < FLAGS_save_delay) {
    cout << "*********************************************" << endl;
    cout << "WARNING: You're only running it for " << num_iterations -
      initial_iteration + 1 << " iterations, but every " <<
      FLAGS_save_delay << "iterations will be saved once," <<
      " so no additional state will be saved." << endl;
    cout << "*********************************************" << endl;
  }

  // std::stringstream fs;
  // fs << "mytest_" << initial_iteration << "_" << num_iterations << ".txt";
  // string filename;
  // fs >> filename;

  // std::ofstream mytest;
  // mytest.open(filename.c_str());
  // mytest << "hello" << endl;
  // mytest.close();

  // if run 0 extra iterations in resume, we simply output the same result
  if ((FLAGS_verbose_assignments || FLAGS_mallet_assignments)
         && initial_iteration == (num_iterations + 1)) {
    cout << "*********************************************" << endl;
    cout << "running for 0 iterations, writing verbose or mallet files" << endl;
    WriteReport();
    // state_->PrintStatus(initial_iteration);
    cout << "Finishing writing!" << endl;
    cout << "*********************************************" << endl;
  }

  for (int ii = initial_iteration; ii <= num_iterations; ++ii) {
    // Make runs reproducible based on the iteration (so that it doesn't matter
    // if we restart a run)
    int seed = FLAGS_rand + ii;
    srand(seed);
    boost::timer time;
    cout << " ------------ Iteration " << ii <<
      "(" << lhood << ") [seed=" << seed <<
         "] ------------ " << endl;

    if (ii > 0 && FLAGS_global_sample_delay >= 1 &&
        ii % FLAGS_global_sample_delay == 0 &&
        ii > FLAGS_global_sample_burnin)
      SampleCorpusVariables(ii, FLAGS_global_sample_step,
                            FLAGS_global_sample_reps,
                            &param_file);

    for (int dd = 0; dd < corpus_->num_docs(); ++dd) {
      // Skip test documents, as we'll do held out lhood on them later
      lib_corpora::MlSeqDoc* doc = corpus_->seq_doc(dd);
      if (IsDocSkippable(doc)) continue;

      int num_words = doc->num_tokens();
      int num_sampled = 0;

      for (int ww = 0; ww < num_words; ++ww) {
        // debug
        // int term = (*doc)[ww];
        // if (term == 144 || term == 47) {
        // cout << "found word" << endl;
        // }
        int topic = SampleWord(dd, ww);
        if (topic != -1)
          num_sampled++;
      }

      if (num_sampled == 0) {
        doc->Print();
        cout << "Warning! Doc " << dd << " had zero words" << endl;
      }

      if (dd % 250 == 0) {
        cout << "Sampling doc " << dd << "/" << corpus_->num_docs() <<
          "(" << num_words << ")" << endl;
      }
      // state_->PrintStatus(ii);
    }
    double elapsed = time.elapsed();

    if (ii > 0 && ii % FLAGS_save_delay == 0) {
      WriteReport();
    }

    lhood = WriteLhoodUpdate(&lhood_file, ii, elapsed);
    lhood_file << endl;
    lhood_file.flush();

    // PrintStatus must happen after sampling corpus variables for MLSLDA or all
    // test documents will have a predicted 0 response
    state_->PrintStatus(ii);
  }
}

double Sampler::WriteLhoodUpdate(std::ostream* lhood_file, int iteration,
                                 double elapsed) {
  // double lhood = TrainLikelihoodIncrement();
  double lhood = Likelihood();

  (*lhood_file) << iteration;
  (*lhood_file) << "\t" << lhood;
  // This doesn't work anymore
  // (*lhood_file) << "\t" << TestLikelihoodIncrement();
  (*lhood_file) << "\t" << TrainLikelihoodIncrement();
  (*lhood_file) << "\t" << elapsed;
  return lhood;
}

void Sampler::set_alpha(double alpha) {
  // Whatever biasing hapened at initialization disappears now
  // TODO(jbg): Keep biasing somehow?
  alpha_zero_ = 1.0;
  alpha_sum_ = alpha;
  state_->SetAlpha(alpha);
}

void Sampler::set_lambda(double lambda) {
  lambda_ = lambda;
  state_->SetLambda(lambda);
}

  /*
   * TODO(jbg): Make it so that this doesn't instantiate a new vector each time
   * it's called.
   */
int Sampler::num_hyperparameters() {
  return hyperparameters().size();
}

vector<string> Sampler::hyperparameter_names() {
  vector<string> names(3);
  names[0] = "iteration";
  names[1] = "alpha";
  names[2] = "lambda";
  return names;
}

  // Returns vector of the log of the hyperparamters
vector<double> Sampler::hyperparameters() {
  vector<double> p(2);
  p[0] = log(alpha_sum_);
  p[1] = log(lambda_);
  return p;
}

  // Sets hyperparameters from vector containing logs of hyperparameters
void Sampler::SetHyperparameters(const vector<double>& hyperparameters) {
  assert(hyperparameters.size() == 2);
  set_alpha(exp(hyperparameters[0]));
  set_lambda(exp(hyperparameters[1]));
}


void Sampler::SampleCorpusVariables(int iter, double step, int samples,
                                    std::ostream* param_file) {
  assert(step >= 0.0);

  vector<double> current = hyperparameters();
  vector<double> temp = hyperparameters();

  int dim = current.size();
  assert(dim > 0);
  vector<double> l(dim);
  vector<double> r(dim);

  double lp = 0.0;
  for (int ii = 0; ii < samples; ++ii) {
    SetHyperparameters(current);
    // lp = TrainLikelihoodIncrement();
    lp = Likelihood();
    // BOOST_ASSERT(lp == TrainLikelihoodIncrement());

    cout << "OLD:";
    double goal = lp + log(lib_prob::next_uniform());
    for (int dd = 0; dd < dim; dd++) {
      cout << exp(current[dd]) << " ";
      l[dd] = current[dd] - lib_prob::next_uniform() * step;
      r[dd] = l[dd] + step;
    }
    cout << endl << "lp = " << lp << " Goal = " << goal << endl;

    while (true) {
      cout << ".";
      cout.flush();

      for (int dd = 0; dd < dim; dd++) {
        temp[dd] = l[dd] + lib_prob::next_uniform() * (r[dd] - l[dd]);
      }

      SetHyperparameters(temp);
      // double new_prob = TrainLikelihoodIncrement();
      double new_prob = Likelihood();
      cout << " Hyperparamters iter" << ii << ": " << new_prob
           << " vs. " << goal << " " << lp << endl;
      if (new_prob > goal) {
        break;
      } else {
        for (int dd = 0; dd < dim; dd++) {
          if (temp[dd] < current[dd]) {
            l[dd] = temp[dd];
          } else {
            r[dd] = temp[dd];
          }
          assert(l[dd] <= current[dd]);
          assert(r[dd] >= current[dd]);
          cout << dd << ":(" << exp(l[dd]) << "-" << exp(r[dd]) << ") ";
        }
        cout << endl;
      }
    }

    for (int dd = 0; dd < dim; ++dd) current[dd] = temp[dd];
  }

  SetHyperparameters(current);
  // lp = TrainLikelihoodIncrement();
  lp = Likelihood();
  cout << " Final hyperparameter update " << lp << endl;
  (*param_file) << iter;
  for (int dd = 0; dd < dim; ++dd)
    (*param_file) << "\t" << exp(current[dd]);
  (*param_file) << endl;
  param_file->flush();
}

double Sampler::TrainLikelihoodIncrement() {
  return TrainLikelihoodIncrement(-1);
}

  // Compute the data probability using the current assignments for
  // the first "limit" words.  If limit is -1, then do all words
double Sampler::TrainLikelihoodIncrement(int64_t limit) {
  double val = 0.0;
  state_->ResetCounts();
  int64_t total_words = 0;
  for (int dd = 0; dd < corpus_->num_docs(); ++dd) {
    lib_corpora::MlSeqDoc* doc = corpus_->seq_doc(dd);
    if (IsDocSkippable(doc)) continue;
    int num_words = doc->num_tokens();
    for (int ww = 0; ww < num_words; ++ww) {
      if (limit >= 0 && total_words++ >= limit) break;
      val += LogProbIncrement(dd, ww);
      // cout << "[LPI] " << total_words << " " << val << endl;
    }
  }
  return val;
}

  /*
   * This is an approximation of Wallach's left-to-right method.  It samples a
   * topic for each word in a document, adds that to the log likelihood so far,
   * then resets the count of that document.
   *
   * TODO(jbg): This is not well-tested.
   */
double Sampler::TestLikelihoodIncrement() {
  double val = 0.0;
  for (int dd = 0; dd < corpus_->num_docs(); ++dd) {
    lib_corpora::MlSeqDoc* doc = corpus_->seq_doc(dd);
    int num_words = doc->num_tokens();
    if (!IsDocSkippable(doc)) continue;
    for (int ww = 0; ww < num_words; ++ww) {
      val += LogProbLeftToRightIncrement(dd, ww);
    }
    ResetTest(dd);
  }
  return val;
}

void Sampler::ResetTest(int doc) {
  int num_words = corpus_->seq_doc(doc)->num_tokens();
  for (int ww = 0; ww < num_words; ++ww) {
    state_->ChangeTopic(doc, ww, -1);
  }
}

double Sampler::LogProbIncrement(int doc, int index) {
  int topic = state_->topic_assignment(doc, index);
  assert(topic != -1);
  // cout << "Fallback doc: " << exp(state_->TopicDocProbability(doc, topic));
  double val = TopicProbability(doc, index, topic);
  // cout << "With term prob: " << exp(val) << endl;
  state_->ChangeTopicCount(doc, index, topic, +1);
  return val;
}

  /*
   * With this method, we sample as we go, so we need it to be initially
   * unassigned
   */
double Sampler::LogProbLeftToRightIncrement(int doc, int index) {
  int topic = state_->topic_assignment(doc, index);
  assert(topic == -1);
  SampleWord(doc, index);
  topic = state_->topic_assignment(doc, index);

  double val = lib_prob::safe_log(0.0);

  if (topic != -1) {
    for (int kk = 0; kk < num_topics_; ++kk)
      val = lib_prob::log_sum(TopicProbability(doc, index, topic), val);
  }
  return val;
}

  /* 
   * This method has been deprecated, as it has prooved too difficult to put
   * into downstream models.
   */
double Sampler::Likelihood() {
  double val = 0;
  val += state_->DocLikelihood();
  val += state_->TopicLikelihood();
  return val;
}


Sampler::~Sampler() {}

MultilingSampler::MultilingSampler(double alpha_sum,
                                   double alpha_zero,
                                   double lambda,
                                   int num_topics) : Sampler(alpha_sum,
                                                             alpha_zero,
                                                             lambda,
                                                             num_topics) {}

double MultilingSampler::TopicProbability(int doc, int index, int topic) {
  const lib_corpora::MlSeqDoc* d = corpus_->seq_doc(doc);
  int lang = d->language();
  int term = (*d)[index];
  double doc_val = state_->TopicDocProbability(doc, topic);
  double voc_val = static_cast<MultilingState*>(state_.get())->
    TopicVocProbability(lang, topic, term);

  BOOST_ASSERT(!isnan(doc_val));
  BOOST_ASSERT(!isnan(voc_val));

  return doc_val + voc_val;
}

void MultilingSampler::InitializeState(const string& output_location,
                                       bool random_init) {
  cout << "Creating new Multilingual State" << endl;
  state_.reset(new MultilingState(num_topics_,
                                  output_location));
  state_->Initialize(corpus_, random_init, alpha_sum_, alpha_zero_, lambda_);
}

void MultilingSampler::WriteReport() {
  Sampler::WriteReport();
  /*
   * Removed until corpora has this capability again
  static_cast<lda::TaggedCorpus*>
    (corpus_.get())->WriteAnnotations(state_->output_path() + ".doc_anno");
  */
}
}  // namespace lda_sampler
