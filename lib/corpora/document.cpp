/*
 * Copyright 2009 Author: Jordan Boyd-Graber
 */
#include "topicmod/lib/corpora/document.h"
#include <string>

namespace lib_corpora {

BaseDoc::BaseDoc(bool is_test, int id) : is_test_(is_test), id_(id) {
  num_tokens_ = 0;
}

void BaseDoc::Print() {
  cout << "Total: " << num_tokens_ << endl;
}

BaseDoc::~BaseDoc() {
}

void BaseDoc::set_title(string title) {
  title_ = title;
}

const string& BaseDoc::title() const {
  return title_;
}

ExchDoc::ExchDoc(bool is_test, int id) : BaseDoc(is_test, id) {
  num_types_ = 0;
}

void ExchDoc::AddWord(int sentence, int word) {
  // Check for valid input
  if (sentence < 0) throw InvalidIndex(sentence);
  if (word < 0) throw InvalidIndex(word);

  if (counts_.find(word) == counts_.end()) {
    ++num_types_;
  }
  ++counts_[word];
  ++num_tokens_;
}

SeqDoc::SeqDoc(bool is_test, int id) : BaseDoc(is_test, id) {}

void SeqDoc::Print() {
  cout << "Words: ";
  for (int ii = 0; ii < (int)words_.size(); ++ii) {
    cout << words_[ii] << " ";
  }
  cout << endl;
  BaseDoc::Print();
}

void SeqDoc::AddWord(int sentence, int word) {
  num_tokens_ += 1;
  words_.push_back(word);
  // tfidf_.push_back(word);
  BOOST_ASSERT(num_tokens_ == (int)words_.size());
}

void SeqDoc::Addtfidf(int sentence, double tfidf) {
  tfidf_.push_back(tfidf);
  BOOST_ASSERT(num_tokens_ == (int)tfidf_.size());
}

uint16_t SeqDoc::operator[] (int idx) const {
  BOOST_ASSERT(idx < (int)words_.size() && idx >= 0);
  return words_[idx];
}

double SeqDoc::gettfidf(int idx) const {
  BOOST_ASSERT(idx < (int)tfidf_.size() && idx >= 0);
  return tfidf_[idx];
}

MlSeqDoc::MlSeqDoc(bool is_test, int id, int language) : SeqDoc(is_test, id) {
  language_ = language;
}

void MlSeqDoc::Print() {
  cout << "Language: " << language_ << endl;
  SeqDoc::Print();
}

SemMlSeqDoc::SemMlSeqDoc(bool is_test, int id, int language)
  : MlSeqDoc(is_test, id, language) {}

int SemMlSeqDoc::synset(int index) const {
  BOOST_ASSERT(index < (int)synsets_.size());
  return synsets_[index];
}

void SemMlSeqDoc::AddWord(int sentence, int word, int synset) {
  synsets_.push_back(synset);
  MlSeqDoc::AddWord(sentence, word);
}

ReviewDoc::ReviewDoc(bool is_test, int id, int language,
                     int author, double rating)
  : MlSeqDoc(is_test, id, language) {
  author_ = author;
  rating_ = rating;
}

void ReviewDoc::Print() {
  cout << "Rating: " << rating_ << endl;
  MlSeqDoc::Print();
}

int MlSeqDoc::language() const {
  return language_;
}

int ReviewDoc::author() const {
  return author_;
}

double ReviewDoc::rating() const {
  return rating_;
}

ExchSentDoc::ExchSentDoc(bool is_test, int id) : ExchDoc(is_test, id) {
  num_sentences_ = 0;
}

void ExchSentDoc::AddWord(int sentence, int word) {
  ExchDoc::AddWord(sentence, word);
  if (sentence > num_sentences_)
    throw SentenceOutOfOrder(sentence, num_sentences_);

  if (sentence == num_sentences_) {
    ++num_sentences_;
    num_sentence_tokens_.push_back(0);
    num_sentence_types_.push_back(0);
    sentence_counts_.resize(num_sentences_);
  }
  ++sentence_counts_[sentence][word];
  ++num_sentence_tokens_[sentence];
  if (sentence_counts_[sentence].find(word) ==
      sentence_counts_[sentence].end()) {
    ++num_sentence_types_[sentence];
  }
  BOOST_ASSERT(num_sentences_ == (int)num_sentence_tokens_.size());
  BOOST_ASSERT(num_sentences_ == (int)num_sentence_types_.size());
  BOOST_ASSERT(num_sentences_ == (int)sentence_counts_.size());
}

int ExchSentDoc::num_sentence_tokens(int sentence) {
  BOOST_ASSERT(sentence < num_sentences_);
  BOOST_ASSERT(sentence >= 0);
  return num_sentence_tokens_[sentence];
}

int ExchSentDoc::num_sentence_types(int sentence) {
  BOOST_ASSERT(sentence < num_sentences_);
  BOOST_ASSERT(sentence >= 0);
  return num_sentence_types_[sentence];
}

const IntMap& ExchSentDoc::sentence_counts(int sentence) {
  BOOST_ASSERT(sentence < num_sentences_);
  BOOST_ASSERT(sentence >= 0);
  return sentence_counts_[sentence];
}
}
