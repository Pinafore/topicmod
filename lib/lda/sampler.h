/*
 * sampler.h
 *
 * Copyright 2009, Jordan Boyd-Graber
 *
 * Basic functionality to sample the states in an lda-style model
 */

#ifndef TOPICMOD_LIB_LDA_SAMPLER_H__
#define TOPICMOD_LIB_LDA_SAMPLER_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "boost/scoped_array.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/timer.hpp"

#ifdef OMP
#include <omp.h> // NOLINT
#endif

#include "topicmod/lib/lda/state.h"
#include "topicmod/lib/prob/logspace.h"
#include "topicmod/lib/util/flags.h"
#include "topicmod/lib/corpora/corpus.h"

namespace lib_lda {

using std::ios;
using lib_prob::log_vector_sample;

class Sampler {
 public:
  Sampler(double alpha_sum,
          double alpha_zero,
          double lambda,
          int num_topics);

  double TrainLikelihoodIncrement();
  double TrainLikelihoodIncrement(int64_t limit);
  virtual double TestLikelihoodIncrement();
  virtual double Likelihood();
  int num_hyperparameters();
  const string& output_path();
  int rand_seed();

  virtual void Initialize(lib_corpora::MlSeqCorpus* corpus);
  void RunSampler(int num_iterations);
  virtual ~Sampler();

 protected:

  // Functions called by the initialization code
  virtual bool IsDocSkippable(lib_corpora::MlSeqDoc* doc);
  virtual void InitializeState(const string& output_location, bool random_init);
  virtual void InitializeTemp();

  // Functions related to sampling
  virtual double TopicProbability(int doc,
                                  int index,
                                  int topic);
  virtual void PrepareWord(int doc, int index);
  int SampleWord(int doc, int index);
  virtual int SampleWordTopic(int doc, int index);
  virtual void SampleCorpusVariables(int iter, double step, int samples,
                                     std::ostream* lhood_file);
  virtual double LogProbIncrement(int doc, int index);
  virtual double LogProbLeftToRightIncrement(int doc, int index);
  virtual void ResetTest(int doc);

  virtual void WriteReport();
  virtual double WriteLhoodUpdate(std::ostream* lhood_file, int iteration,
                                  double elapsed);

  // Update hyperparmaters (called by Sample Corpus Variables)
  virtual void SetHyperparameters(const vector<double>& hyperparameters);
  virtual vector<double> hyperparameters();
  virtual vector<string> hyperparameter_names();
  void set_alpha(double alpha);
  void set_lambda(double lambda);

  int iteration_;
  int num_threads_;

  // Parameters
  double alpha_sum_;
  double alpha_zero_;
  double lambda_;
  int num_topics_;
  bool params_not_default_;
  bool hyperparameter_file_;

  // Temporary array for sampling
  std::vector<double> temp_probs_;

  // The corpus pointer is owned by the state, just like health care in the UK
  lib_corpora::MlSeqCorpus* corpus_;
  boost::scoped_ptr<State> state_;
 private:
  // No evil copy constructors
  Sampler(const Sampler&);
};

class MultilingSampler : public Sampler {
 public:
  friend void lhood();
  MultilingSampler(double alpha_sum,
                   double alpha_zero,
                   double lambda,
                   int num_topics);

  int vocab_limit() const;
  void set_vocab_limit(int vocab_limt);
  // Without arguments, it sets it using user-defined flags
  void set_vocab_limit();

 protected:
  double TopicProbability(int doc,
                          int index,
                          int topic);

  void InitializeState(const string& output_location, bool random_init);
  void WriteReport();
};
}
#endif  // TOPICMOD_LIB_LDA_SAMPLER_H_
