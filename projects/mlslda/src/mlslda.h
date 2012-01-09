/*
 * ldawn.h
 *
 * Copyright 2007, Jordan Boyd-Graber
 *
 * LDA derivative that uses distributions over trees instead of regular topics
 */

#ifndef __PROJECT_MLSLDA_SRC_MLSLDA_H__
#define __PROJECT_MLSLDA_SRC_MLSLDA_H__

#include <boost/foreach.hpp>
#include <gsl/gsl_math.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_sort_vector.h>
#include <string>
#include <fstream>
#include <vector>

#include "topicmod/projects/ldawn/src/ldawn.h"

// Forward declaration
namespace topicmod_projects_mlslda_test {
  void test_topic_matrix();
  void test_regression();
  void test_sampling_probs();
}

namespace topicmod_projects_mlslda {

using lib_corpora::MlSeqDoc;
using lib_corpora::SemMlSeqDoc;
using std::ios;
using std::fstream;

struct RegressionWorkspace {
  double chi_squared_;
  boost::shared_ptr<gsl_matrix> x_;
  boost::shared_ptr<gsl_matrix> covariance_;
  boost::shared_ptr<gsl_multifit_linear_workspace> space_;
};

class MlSldaState : public topicmod_projects_ldawn::LdawnState {
  public:
    friend void topicmod_projects_mlslda_test::test_topic_matrix();
    friend void topicmod_projects_mlslda_test::test_regression();
    friend void topicmod_projects_mlslda_test::test_sampling_probs();
    friend class MlSlda;
    MlSldaState(int num_topics, string output, string wn_location,
                bool logistic);
    double sigma_squared() const;
    void set_sigma_squared(double sigma_squared);
    double prediction(int doc) const;
    void set_prediction(int doc, double prediction);
    double ComputeResponseProbability(int doc);
    double TopicDocProbability(int doc, int topic);
    double TopicDocResponse(double nu, double num_docs, double v,
                            double y, double sigma_squared,
                            bool logistic) const;
    void ChangePath(int doc, int index, int topic, int path);
    void ChangeTopic(int doc, int index, int topic);
    const gsl_vector* nu() const;
  protected:
    virtual void InitializeOther();
    virtual void InitializeRegression();
    virtual void InitializeResponse();
    virtual void InitializeAssignments(bool random_init);
    virtual void InitializeLength();

    double OptimizeRegression();
    double ComputeError(bool use_training);
    void UpdatePrediction(int doc, int index, int topic);
    void UpdateDocPrediction(int doc);
    void UpdateAllPredictions();
    void train_topic_assignments(gsl_matrix* z) const;
    void WriteTopicHeader(int topic, std::fstream* outfile);
    void ReadAssignments();
    double WriteDisambiguation(int iteration) const;
    void WriteNu();
    boost::shared_ptr<gsl_vector> nu_;
    vector<double> v_;
    vector<double> truth_;
    bool logistic_;
    vector<double> effective_length_;
    double sigma_squared_;
    double log_normal_normalizer_;
    RegressionWorkspace regression_space_;
};

class MlSlda : public topicmod_projects_ldawn::Ldawn {
 public:
  MlSlda(double alpha_sum, double alpha_zero, double lambda,
         int num_topics, string wn_location);
  double TestLikelihoodIncrement();
  bool regression_only();
  bool update_hyperparameters() const;
  bool logistic() const;
  double variance() const;
  double Likelihood();

  virtual void SetHyperparameters(const vector<double>& hyperparameters);
  virtual vector<double> hyperparameters();
  virtual vector<string> hyperparameter_names();
 protected:
  virtual bool IsDocSkippable(MlSeqDoc* doc);
  virtual void InitializeState(const string& output_location, bool random_init);
  virtual double WriteLhoodUpdate(std::ostream* lhood_file, int iteration,
                                  double elapsed);
  virtual void SampleCorpusVariables(int iter, double step, int samples,
                                     std::ostream* param_file);
  virtual void ResetTest(int doc);
  virtual void WriteReport();
};

inline double MlSldaState::TopicDocResponse(double nu, double num_words,
                                            double v, double y,
                                            double sigma_squared,
                                            bool logistic) const {
  // Copying logic of Jonathan's sampling function
  double val = nu / num_words;
  if (logistic) {
    val -= v;
    if (y > 0.0) val = -val;
    val = -log(1.0 + exp(val));
  } else {
    val = val * (v - (val / 2.0));
    // Doesn't vary across topics
    // val -= 2 * v * v;
    val /= sigma_squared;
  }

  /* Old version
  double val = v / (double)num_words;
  // cout << "1 " << val << " (v=" << v << ", num_words=" << num_words <<
  // ")" << endl;
  val += y;
  // cout << "2 " << val << " (y=" << y << ")" << endl;
  val *= nu / (num_words * sigma_squared);
  // cout << "3 " << val << endl;
  val -= pow(nu / num_words, 2.0) / (2.0 * sigma_squared);
  // cout << "4 " << val << endl;
  */
  // val += log_normal_normalizer_;
  return val;
}
}
#endif
