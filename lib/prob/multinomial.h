/*
 * multinomial.h
 *
 * Copyright 2008, Jordan Boyd-Graber
 *
 * Class to track counts of draws from multinomial distributions
 */

#ifndef TOPICMOD_LIB_PROB_MULTINOMIAL_H_
#define TOPICMOD_LIB_PROB_MULTINOMIAL_H_

#include <assert.h>

#include <gsl/gsl_blas.h>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include <algorithm>
#include <map>
#include <utility>
#include <string>
#include <vector>

#include "topicmod/lib/util/strings.h"
#include "topicmod/lib/prob/logspace.h"

namespace lib_prob {

using std::map;
using std::pair;
typedef map<int, int> IntMap;
typedef map<int, string> Dict;

class Multinomial {
 public:
  Multinomial();
  virtual ~Multinomial();

  void Initialize(int size, double prior_sum);
  virtual double LogLikelihood() const = 0;
  virtual void Reset() = 0;
  double Probability(int item) const;
  double Delta_LogProb(int item);
  virtual double LogNumerator(int item) const = 0;
  virtual double LogProbability(int item) const = 0;
  virtual void ChangeCount(int item, int count) = 0;
  virtual void SetPriorSum(double val) = 0;
  void SetPriorScale(double scale);
  virtual double prior_sum() const;
  virtual int count(int item) const = 0;
  void WriteTopWords(int num_to_write,
                     const vector<string>& vocab,
                     std::ostream* out);
  string debug_string(int item) const;
  int sum() const;
  int size() const;

 protected:
  int size_;
  int sum_;
  double prior_sum_;
  double prior_val_;
  virtual void FillProbArray(vector< pair<double, int> >* probs) = 0;
};

/*
 * A class that represents "no choice"; there is only one discrete option.  Used
 * to optimize speed and memory.
 */
class DegenerateMultinomial : public Multinomial {
 public:
  DegenerateMultinomial() {sum_ = 0;}
  void Initialize(int size, double prior_sum) {sum_ = 0;}
  double LogLikelihood() const {return 0.0;}
  void Reset() {sum_ = 0;}
  double LogNumerator(int item) const {return 0.0;}
  double LogProbability(int item) const {return 0.0;}
  void ChangeCount(int item, int count) {sum_ += count;}
  void SetPriorSum(double val) {}
  double prior_sum() const {return 0.0;}
  int count(int item) const {return sum_;}
 protected:
  void FillProbArray(vector< pair<double, int> >* probs) {}
};

class DenseMultinomial : public Multinomial {
 public:
  DenseMultinomial();
  void Initialize(boost::shared_ptr<gsl_vector> const & prior,
                  double prior_sum);
  void Initialize(int size, double prior_sum);
  // Shares ownership
  void SetPriorProportion(boost::shared_ptr<gsl_vector> const & prior);
  // Takes over ownership
  void SetPriorProportion(gsl_vector* prior);
  virtual void SetPriorSum(double val);

  void Reset();
  double LogLikelihood() const;
  double LogProbability(int item) const;
  double LogNumerator(int item) const;
  void ChangeCount(int item, int count);
  int count(int item) const;
  string debug_string(int item) const;
  ~DenseMultinomial();
 private:
  boost::shared_ptr<gsl_vector> prior_;
  std::vector<int> counts_;
  bool use_explicit_prior_;
  void FillProbArray(vector< pair<double, int> >* probs);
  void CheckPrior();
  bool degenerate_;
};

class CachedMultinomial : public DenseMultinomial {
 public:
  CachedMultinomial();
  double LogProbability(int item);
  void ChangeCount(int item, int count);
  void Initialize(boost::shared_ptr<gsl_vector> const & prior,
                  double prior_sum);
  void Initialize(int size, double prior_sum);
  void Reset();
 private:
  void ResetCache();
  void InitializeCache(int size);
  std::vector<bool> valid_;
  std::vector<double> cache_;
};

class SparseMultinomial : public Multinomial {
 public:
  SparseMultinomial();
  virtual void SetPriorSum(double val);
  double LogLikelihood() const;
  double LogProbability(int item) const;
  double LogNumerator(int item) const;
  int count(int item) const;
  void Reset();
  void ChangeCount(int item, int count);
  ~SparseMultinomial();
 private:
  void FillProbArray(vector< pair<double, int> >* probs);
  map<int, int> counts_;
};

typedef DenseMultinomial VocDist;
}

#endif  // TOPICMOD_LIB_PROB_MULTINOMIAL_H_
