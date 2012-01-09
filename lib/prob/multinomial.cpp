/*
 * mutlinomial.cpp
 *
 * Copyright 2009, Jordan Boyd-Graber
 *
 * Class for storing the counts of and computing the posterior probabilities for
 * a multinomial distribution.  Has both sparse and dense versions, informed and
 * uninformed priors.
 */

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "topicmod/lib/prob/multinomial.h"

namespace lib_prob {

Multinomial::Multinomial() {
  size_ = 0;
  sum_ = 0;
  prior_sum_ = 0;
  prior_val_ = 0;
}

void Multinomial::SetPriorScale(double scale) {
  double new_sum = (double)size_ * scale;
  SetPriorSum(new_sum);
}

double Multinomial::Delta_LogProb(int item) {
  if (sum_ == 0) {
    return log(1.0 / size_);
  } else {
    if (sum_ == count(item))
      return 0.0;
    else
      return -1e99;
  }
}

double Multinomial::prior_sum() const {
  return prior_sum_;
}

int Multinomial::sum() const {
  return sum_;
}

int Multinomial::size() const {
  return size_;
}

void Multinomial::Initialize(int size, double prior_sum) {
  assert(size > 0);
  assert(prior_sum > 0.0);
  size_ = size;
  prior_sum_ = prior_sum;
  prior_val_ = prior_sum / (double)size;
}

Multinomial::~Multinomial() {}

DenseMultinomial::DenseMultinomial() : Multinomial() {
  use_explicit_prior_ = false;
  degenerate_ = false;
}

void DenseMultinomial::SetPriorProportion(boost::shared_ptr<gsl_vector>
                                          const & prior) {
  prior_ = prior;
  use_explicit_prior_ = true;
  CheckPrior();
}

void DenseMultinomial::Initialize(boost::shared_ptr<gsl_vector>
                                  const & prior,
                                  double prior_sum) {
  Multinomial::Initialize(prior->size, prior_sum);
  counts_.resize(size_);
  SetPriorProportion(prior);
}

void DenseMultinomial::Reset() {
  std::fill(counts_.begin(), counts_.end(), 0);
  sum_ = 0;
}

void DenseMultinomial::SetPriorProportion(gsl_vector* prior) {
  prior_.reset(prior, gsl_vector_free);
  use_explicit_prior_ = true;
  CheckPrior();
}

string DenseMultinomial::debug_string(int item) const {
  string val = "(" + lib_util::SimpleItoa(count(item)) + " + ";
  if (use_explicit_prior_) {
    val += lib_util::SimpleDtoa(gsl_vector_get(prior_.get(), item));
  } else {
    val += lib_util::SimpleDtoa(prior_val_);
  }
  val += ") / (" + lib_util::SimpleItoa(sum()) + " + " +
    lib_util::SimpleDtoa(prior_sum_) + ")";
  return val;
}

double Multinomial::Probability(int item) const {
  return exp(LogProbability(item));
}

void DenseMultinomial::SetPriorSum(double val) {
  prior_sum_ = val;
  if (use_explicit_prior_) {
    CheckPrior();
  } else {
    prior_val_ = prior_sum_ / ((double) size_);
  }
}

void DenseMultinomial::CheckPrior() {
  int num_elements_equal_zero = 0;

  assert(use_explicit_prior_);
  double sum = gsl_blas_dasum(prior_.get());
  assert(sum > 0.0);
  if (sum != prior_sum_) {
    gsl_blas_dscal(prior_sum_ / sum, prior_.get());
  }

  for (int ii = 0; ii < (int)prior_->size; ++ii) {
    double val = gsl_vector_get(prior_.get(), ii);
    if (val == 0.0) num_elements_equal_zero++;
    assert(val >= 0.0);
  }

  assert(num_elements_equal_zero <= 1);
  if (num_elements_equal_zero == 1) {
    degenerate_ = true;
    assert(size_ <= 2);  // If we remove this assumption, then computation of
                         // log probability has to be changed so that it doesn't
                         // short circuit when the distribution is degenerate.
  } else {
    degenerate_ = false;
  }

  // cout << "prior_sum= " << prior_sum_ << " size= " << size_ << " sum= " <<
  // sum << " now = " << gsl_blas_dasum(prior_.get()) << endl;
}

double DenseMultinomial::LogNumerator(int item) const {
  double numerator = counts_[item];
  if (use_explicit_prior_) {
    numerator += gsl_vector_get(prior_.get(), item);
  } else {
    numerator += prior_val_;
  }
  BOOST_ASSERT(numerator > 0.0);
  double val = log(numerator);
  // cout << "LN:" << numerator << " " << val << " " << log(numerator) << endl;
  BOOST_ASSERT(!isnan(val));
  return val;
}

void CachedMultinomial::Initialize(boost::shared_ptr<gsl_vector>
                                  const & prior,
                                  double prior_sum) {
  InitializeCache(prior->size);
  DenseMultinomial::Initialize(prior, prior_sum);
}

void CachedMultinomial::Initialize(int size, double prior_sum) {
  InitializeCache(size);
  DenseMultinomial::Initialize(size, prior_sum);
}

void CachedMultinomial::InitializeCache(int size) {
  cache_.resize(size);
  valid_.resize(size);
  ResetCache();
}

void CachedMultinomial::ChangeCount(int item, int count) {
  ResetCache();
  DenseMultinomial::ChangeCount(item, count);
}

double CachedMultinomial::LogProbability(int item) {
  if (valid_[item]) {
    return cache_[item];
  } else {
    double val = DenseMultinomial::LogProbability(item);
    cache_[item] = val;
    valid_[item] = true;
    return val;
  }
}

CachedMultinomial::CachedMultinomial() {}

void CachedMultinomial::Reset() {
  ResetCache();
  DenseMultinomial::Reset();
}

void CachedMultinomial::ResetCache() {
  std::fill(valid_.begin(), valid_.end(), false);
}

double DenseMultinomial::LogProbability(int item) const {
  // Skip distributions that really aren't

  // TODO(jbg): If we check if the distribution is degenerate and just return
  // 0.0, then we might be able to speed things up.
  if (size_ <= 1 || degenerate_) {
    return 0.0;
  }

  BOOST_ASSERT(item < size_);

  double numerator = counts_[item];
  if (use_explicit_prior_) {
    numerator += gsl_vector_get(prior_.get(), item);
  } else {
    numerator += prior_val_;
  }
  double denominator = sum_ + prior_sum_;

  /*
  cout << "(" << counts_[item] <<  " + " << prior_val_ << ") / " <<
    "(" << sum_ << " + " << prior_sum_ << ")" <<
    ":" << size_ << endl;

  cout << log(numerator) << " " << log(denominator) << " " <<
    log(numerator) - log(denominator) << endl;
  */

  double val = log(numerator);
  BOOST_ASSERT(!isnan(val));
  val -= log(denominator);
  BOOST_ASSERT(!isnan(val));

  return val;
}

void DenseMultinomial::Initialize(int size, double prior_sum) {
  Multinomial::Initialize(size, prior_sum);
  counts_.resize(size);
}

double DenseMultinomial::LogLikelihood() const {
  if (size_ == 0 || degenerate_) return 0.0;
  if (use_explicit_prior_) {
    return log_dirichlet_likelihood(sum_,
                                    prior_sum_,
                                    prior_.get(),
                                    counts_);
  } else {
    return log_dirichlet_likelihood(sum_,
                                    prior_sum_,
                                    counts_);
  }
}

string Multinomial::debug_string(int item) const {
  string val = "(" + lib_util::SimpleItoa(count(item)) +
    " + " + lib_util::SimpleDtoa(prior_val_) +
    ") / (" + lib_util::SimpleItoa(sum()) + " + " +
    lib_util::SimpleDtoa(prior_sum_) + ")";
  return val;
}

void DenseMultinomial::ChangeCount(int item, int count) {
  BOOST_ASSERT(item < (int)counts_.size());
  counts_[item] += count;
  sum_ += count;
  BOOST_ASSERT(counts_[item] >= 0);
  BOOST_ASSERT(sum_ >= 0);
}

int DenseMultinomial::count(int item) const {
  return counts_[item];
}

DenseMultinomial::~DenseMultinomial() {}

SparseMultinomial::SparseMultinomial() : Multinomial() {}

SparseMultinomial::~SparseMultinomial() {}

void SparseMultinomial::ChangeCount(int item, int count) {
  counts_[item] += count;
  sum_ += count;
  assert(counts_[item] >= 0);

  // TODO(jbg): Maybe consider removing zero counts?
}

void SparseMultinomial::Reset() {
  sum_ = 0;
  counts_.clear();
}

int SparseMultinomial::count(int item) const {
  if (counts_.find(item) != counts_.end()) {
    return counts_.find(item)->second;
  } else {
    return 0;
  }
}

void SparseMultinomial::FillProbArray(vector< pair<double, int> >* probs) {
  BOOST_FOREACH(IntMap::value_type ii, counts_) {
    int id = ii.first;
    probs->push_back(pair<double, int>(LogProbability(id), id));
  }
}

void DenseMultinomial::FillProbArray(vector< pair<double, int> >* probs) {
  for (int ii = 0; ii < (int)counts_.size(); ++ii) {
    probs->push_back(pair<double, int>(LogProbability(ii), ii));
  }
}

void Multinomial::WriteTopWords(int num_to_write,
                                const vector<string>& vocab,
                                std::ostream* out) {
  vector< pair<double, int> > probs;
  FillProbArray(&probs);
  if ((int)probs.size() < num_to_write) {
    num_to_write = probs.size();
  }

  sort(probs.rbegin(), probs.rend());
  for (int ii = 0; ii < num_to_write; ++ii) {
    int id = probs[ii].second;
    if (count(id) > 0) {
      (*out) << exp(probs[ii].first) << " " << id << " " << count(id) << " " <<
        vocab[id] << endl;
    }
  }
}

double SparseMultinomial::LogLikelihood() const {
  int keys_seen = 0;
  double val = 0.0;

  val += gsl_sf_lngamma(prior_sum_);
  val -= (double)size_ * gsl_sf_lngamma(prior_val_);

  BOOST_FOREACH(IntMap::value_type ii, counts_) {
    ++keys_seen;
    val += gsl_sf_lngamma(prior_val_ + ii.second);
  }
  // Account for all of the keys we didn't see
  assert(size_ >= keys_seen);
  val += (size_ - keys_seen) * gsl_sf_lngamma(prior_val_);

  val -= gsl_sf_lngamma(prior_sum_ + (double)sum_);
  return val;
}

double SparseMultinomial::LogNumerator(int item) const {
  if (size_ <= 1) return 0.0;

  double numerator = prior_val_;
  IntMap::const_iterator ii = counts_.find(item);
  if (ii != counts_.end()) {
    numerator += ii->second;
  }
  return log(numerator);
}

void SparseMultinomial::SetPriorSum(double val) {
  prior_sum_ = val;
  prior_val_ = prior_sum_ / ((double)size_);
}

double SparseMultinomial::LogProbability(int item) const {
  if (size_ <= 1) {
    return 0.0;
  }

  // We don't just use counts_[item] because this is a const method
  double numerator = prior_val_;
  IntMap::const_iterator ii = counts_.find(item);
  if (ii != counts_.end()) {
    numerator += ii->second;
  }

  double denominator = sum_ + prior_sum_;
  // cout << numerator << " " << denominator << "(" << sum_ << "," <<
  // prior_sum_ << ")" << endl;

  double val = log(numerator) - log(denominator);
  BOOST_ASSERT(!isnan(val));
  return val;
}
}
