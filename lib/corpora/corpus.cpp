/*
 * Copyright 2009 Jordan Boyd-Graber
 */
#include <map>
#include <string>
#include <vector>

#include "topicmod/lib/corpora/corpus.h"

namespace lib_corpora {

Corpus::Corpus() {
  min_training_count_ = 1;
}

Corpus::~Corpus() {
}

void Corpus::WordSeen(int lang, int word, bool is_test) {
  assert(lang < (int)test_count_.size());

  if (is_test) {
    test_count_[lang][word] += 1;
  } else {
    train_count_[lang][word] += 1;
  }
}

void Corpus::AddDoc(BaseDoc* raw) {
  DocPtr d(raw);
  bool test = raw->is_test();
  if (test) {
    test_.push_back(d);
  } else {
    train_.push_back(d);
  }
  full_.push_back(d);
  BOOST_ASSERT(test_.size() <= full_.size());
  BOOST_ASSERT(train_.size() <= full_.size());
  num_tokens_ += d->num_tokens();
}

void Corpus::UpdateMapping(const map<string, int>& author_lookup,
                           const map<string, int>& synset_lookup) {
  authors_.resize(author_lookup.size());
  synsets_.resize(synset_lookup.size());

  BOOST_FOREACH(StrToInt_it& ii, author_lookup) // NOLINT
    authors_[ii.second] = ii.first;

  BOOST_FOREACH(StrToInt_it& ii, synset_lookup) // NOLINT
    synsets_[ii.second] = ii.first;
}

BaseDoc* Corpus::doc(int index) const {
  BOOST_ASSERT(index < (int)full_.size());
  return full_[index].get();
}

int Corpus::num_docs() const {
  BOOST_ASSERT(num_train() + num_test() == (int)full_.size());
  return full_.size();
}

int Corpus::num_train() const {
  return train_.size();
}

int Corpus::num_test() const {
  return test_.size();
}

int Corpus::min_training_count() {
  return min_training_count_;
}

int Corpus::vocab_size(int lang) {
  return vocab_size_[lang];
}

void Corpus::set_min_training_count(int count) {
  min_training_count_ = count;
}

bool Corpus::IsValidWord(int lang, int word) {
  return train_count_[lang][word] >= min_training_count_;
}

int64_t Corpus::num_tokens() const {
  return num_tokens_;
}

int Corpus::num_languages() const {
  return vocab_size_.size();
}

MlSeqCorpus::MlSeqCorpus(MlVocab* vocab) : Corpus() {
  vocab_.reset(vocab);
  int num_langs = vocab->size();
  vocab_size_.resize(num_langs);

  for (int ii = 0; ii < (int)vocab->size(); ++ii) {
    vocab_size_[ii] = (*vocab)[ii].size();
  }
  train_count_.resize(num_langs);
  test_count_.resize(num_langs);
  num_ml_tokens_.resize(num_langs);
}

  /*
   * If the lookup doesn't have the synset, then add it.  Return the id of the
   * synset.  Also add new synsets to the list of synsets.
   */
int Corpus::CheckSynset(const string& synset_key, StrToInt* synset_lookup) {
  int id = synsets_.size();
  if (synset_lookup->find(synset_key) == synset_lookup->end())
    (*synset_lookup)[synset_key] = id;
  else
    id = (*synset_lookup)[synset_key];
  synsets_.push_back(synset_key);
  BOOST_ASSERT((*synset_lookup)[synset_key] == id);
  BOOST_ASSERT(synsets_[id] == synset_key);
  return id;
}

void SemMlSeqCorpus::CreateSynsetVocab(vector<StrToInt>* word_lookup,
                                       StrToInt* synset_lookup) {
  int num_langs = vocab_->size();
  word_lookup->resize(num_langs);
  for (int ll = 0; ll < num_langs; ++ll) {
    for (int ww = 0; ww < vocab_size_[ll]; ++ww) {
      const string& word = (*vocab_)[ll][ww];
      (*word_lookup)[ll][word] = ww;
    }
  }

  for (int ss = 0; ss < (int)synsets_.size(); ++ss) {
    (*synset_lookup)[synsets_[ss]] = ss;
  }
}

void MlSeqCorpus::AddDoc(BaseDoc* raw) {
  Corpus::AddDoc(raw);
  int lang = static_cast<MlSeqDoc*>(raw)->language();
  if (raw->num_tokens() > 0) {
    // If this assertion is failing, then perhaps the corpus reader hasn't
    // "seen" these words (that is, called the WordSeen function)
    assert(lang < (int)num_ml_tokens_.size());
    num_ml_tokens_[lang] += raw->num_tokens();
  }
}

MlSeqDoc* MlSeqCorpus::seq_doc(int index) const {
  return static_cast<MlSeqDoc*>(doc(index));
}

int64_t MlSeqCorpus::num_ml_tokens(int lang) const {
  return num_ml_tokens_[lang];
}

const string& Corpus::synset(int id) const {
  assert(id >= 0 && id < (int)synsets_.size());
  return synsets_[id];
}

const string& Corpus::author(int id) const {
  assert(id >= 0 && id < (int)authors_.size());
  return authors_[id];
}

SemMlSeqCorpus::SemMlSeqCorpus(MlVocab* vocab) : MlSeqCorpus(vocab) {}

ReviewCorpus::ReviewCorpus(MlVocab* vocab) : MlSeqCorpus(vocab) {
  standardized_rating_initialized_ = false;
}

void ReviewCorpus::StandardizeRatings() {
  standardized_training_ratings_.reset(gsl_vector_alloc(num_train()),
                                       gsl_vector_free);
  if (num_test() > 0)
    standardized_test_ratings_.reset(gsl_vector_alloc(num_test()),
                                     gsl_vector_free);

  // Find out how many documents per language we have
  vector<int> docs_per_language;
  for (int ii = 0; ii < num_docs(); ++ii) {
    ReviewDoc* doc = static_cast<ReviewDoc*>(this->doc(ii));
    unsigned int lang = doc->language();
    while (lang >= docs_per_language.size()) docs_per_language.push_back(0);
    docs_per_language[lang]++;
  }

  // Compute the mean and variance for each language
  vector<double> means(docs_per_language.size(), 0.0);
  vector<double> scale(docs_per_language.size(), 0.0);
  for (int ll = 0; ll < (int)docs_per_language.size(); ++ll) {
    if (docs_per_language[ll] == 0) continue;
    shared_ptr<gsl_vector> lang_ratings(gsl_vector_alloc(docs_per_language[ll]),
                                        gsl_vector_free);

    int doc_id = 0;
    for (int ii = 0; ii < num_docs(); ++ii) {
      ReviewDoc* doc = static_cast<ReviewDoc*>(this->doc(ii));
      if (doc->language() == ll) {
        gsl_vector_set(lang_ratings.get(), doc_id, doc->rating());
        ++doc_id;
      }
    }

    means[ll] = gsl_stats_mean(lang_ratings->data,
                               lang_ratings->stride,
                               lang_ratings->size);
    scale[ll] = gsl_stats_variance_m(lang_ratings->data,
                                     lang_ratings->stride,
                                     lang_ratings->size,
                                     means[ll]);

    // If the variance is zero, something is messed up with the data (and we'll
    // end up dividing by zero): either only one document or they all have the
    // same rating
    if (scale[ll] == 0.0) {
      scale[ll] = 1.0;
      std::cout << "***********************************" << endl;
      std::cout << "***********************************" << endl;
      std::cout << "    WARNING!!! VARIANCE IS ZERO    " << endl;
      std::cout << "         LANG " << ll                << endl;
      std::cout << "***********************************" << endl;
      std::cout << "***********************************" << endl;
    }

    scale[ll] = 1.0 / sqrt(scale[ll]);
  }

  // Create combined vector and standardize
  int train_seen = 0;
  int test_seen = 0;
  for (int ii = 0; ii < num_docs(); ++ii) {
    ReviewDoc* doc = static_cast<ReviewDoc*>(this->doc(ii));
    int lang = doc->language();
    double val = scale[lang] * (doc->rating() - means[lang]);

    if (doc->is_test()) {
      gsl_vector_set(standardized_test_ratings_.get(), test_seen++, val);
    } else {
      gsl_vector_set(standardized_training_ratings_.get(), train_seen++, val);
    }
  }
  assert(train_seen == num_train());
  assert(test_seen == num_test());
  assert(train_seen + test_seen == num_docs());

  double test_mean = gsl_stats_mean(standardized_test_ratings_->data,
                                    standardized_test_ratings_->stride,
                                    standardized_test_ratings_->size);
  test_variance_ = gsl_stats_variance_m(standardized_test_ratings_->data,
                                        standardized_test_ratings_->stride,
                                        standardized_test_ratings_->size,
                                        test_mean);
  for (int ll = 0; ll < (int)docs_per_language.size(); ++ll) {
    std::cout << "Standardizing scores for language " << ll << ".  Mean = " <<
      means[ll] << " inv_sd=" << scale[ll] << std::endl;
  }

  for (int ii = 0; ii < (int)standardized_training_ratings_.get()->size; ++ii)
    BOOST_ASSERT(!isnan(gsl_vector_get(standardized_training_ratings_.get(),
                                       ii)));

  standardized_rating_initialized_ = true;
}

double ReviewCorpus::test_variance() const {
  return test_variance_;
}

  /*
   * Return standardized ratings.  If the standardized ratings have not been
   * initialized, then compute the standardized scores.
   */
const gsl_vector* ReviewCorpus::train_ratings() {
  if (!standardized_rating_initialized_) {
    StandardizeRatings();
  }
  return standardized_training_ratings_.get();
}

const gsl_vector* ReviewCorpus::test_ratings() {
  if (!standardized_rating_initialized_) {
    StandardizeRatings();
  }
  return standardized_test_ratings_.get();
}
}  // ends namespace lib_corpora
