/*
 * Copyright 2009 Jordan Boyd-Graber
 */

#ifndef TOPICMOD_LIB_CORPORA_CORPUS_H_
#define TOPICMOD_LIB_CORPORA_CORPUS_H_

#include <stdint.h>
#include <math.h>

#include <gsl/gsl_vector.h>
#include <gsl/gsl_statistics.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "topicmod/lib/corpora/document.h"
#include "topicmod/lib/prob/logspace.h"

namespace lib_corpora {

using std::map;
using boost::shared_ptr;
typedef boost::shared_ptr<BaseDoc> DocPtr;
typedef std::map<string, int> StrToInt;
typedef const std::pair<string, int> StrToInt_it;
typedef vector<string> Lookup;
typedef vector<Lookup> MlVocab;

class Corpus {
 public:
  Corpus();
  virtual ~Corpus();
  void WordSeen(int language, int word, bool is_test);
  virtual void AddDoc(BaseDoc* raw);

  BaseDoc* doc(int index) const;
  int num_docs() const;
  int num_train() const;
  int num_test() const;

  bool IsValidWord(int lang, int word);
  void set_min_training_count(int count);
  int min_training_count();
  int vocab_size(int lang);
  int64_t num_tokens() const;
  int num_languages() const;
  void UpdateMapping(const map<string, int>& author_lookup,
                     const map<string, int>& synset_lookup);
  int CheckSynset(const string& synset_key, StrToInt* synset_lookup);

  const string& synset(int id) const;
  const string& author(int id) const;
 protected:
  std::vector<DocPtr> test_;
  std::vector<DocPtr> train_;
  std::vector<DocPtr> full_;
  vector<IntMap> train_count_;
  vector<IntMap> test_count_;

  Lookup synsets_;
  Lookup authors_;

  vector<int64_t> num_ml_tokens_;
  vector<int> vocab_size_;
  int min_training_count_;
  int64_t num_tokens_;
 private:
  // Prevent copy and assignment
  Corpus(const Corpus&);
  void operator=(const Corpus&);
};

class MlSeqCorpus : public Corpus {
 public:
  explicit MlSeqCorpus(MlVocab* vocab);
  virtual void AddDoc(BaseDoc* raw);
  MlSeqDoc* seq_doc(int index) const;
  MlVocab* vocab() {return vocab_.get();}
  int64_t num_ml_tokens(int lang) const;
 protected:
  boost::scoped_ptr<MlVocab> vocab_;
};

class SemMlSeqCorpus : public MlSeqCorpus {
 public:
  explicit SemMlSeqCorpus(MlVocab* vocab);
  void CreateSynsetVocab(vector<StrToInt>* word_lookup,
                         StrToInt* synset_lookup);
};

class ReviewCorpus : public MlSeqCorpus {
 public:
  explicit ReviewCorpus(MlVocab* vocab);
  const gsl_vector* train_ratings();
  const gsl_vector* test_ratings();
  double test_variance() const;
 private:
  boost::shared_ptr<gsl_vector> standardized_training_ratings_;
  boost::shared_ptr<gsl_vector> standardized_test_ratings_;
  bool standardized_rating_initialized_;
  void StandardizeRatings();
  IntMap review_count_;
  double test_variance_;
};
}

#endif  // TOPICMOD_LIB_CORPORA_CORPUS_H_
