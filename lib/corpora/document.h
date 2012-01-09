/*
 * Copyright 2009 Author: Jordan Boyd-Graber
 */

#ifndef TOPICMOD_LIB_CORPORA_DOCUMENT_H_
#define TOPICMOD_LIB_CORPORA_DOCUMENT_H_

#include <stdint.h>

#include <boost/assert.hpp>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../util/strings.h"

using std::cout;
using std::endl;
using std::string;

namespace lib_corpora {

struct Time {
  int day_;
  int month_;
  int year_;

  Time(int year = -1, int month = -1, int day = -1) {
    day_ = day;
    month_ = month;
    year_ = year;
  }

  int day() const {
    return day_;
  }

  int month() const {
    return month_;
  }

  int year() const {
    return year_;
  }
};

typedef std::map<uint16_t, uint16_t> IntMap;

class SentenceOutOfOrder: public std::exception {
 public:
  SentenceOutOfOrder(int sent, int tot)  {
    message_ = "We saw sentence " + lib_util::SimpleItoa(sent) +
      " but we've only seen " + lib_util::SimpleItoa(tot) +
      " so far.";
  }
  ~SentenceOutOfOrder() throw() {}
  const char* what() const throw() {return message_.c_str();}

 private:
  string message_;
};

class InvalidIndex: public std::exception {
 public:
  explicit InvalidIndex(int token)  {
    message_ = "Token " + lib_util::SimpleItoa(token) +
      " should be non-negative.";
  }
  ~InvalidIndex() throw() {}
  const char* what() const throw() {return message_.c_str();}

 private:
  string message_;
};

class BaseDoc {
 public:
  BaseDoc(bool is_test, int id);
  virtual ~BaseDoc();
  virtual void AddWord(int sentence, int word) = 0;
  virtual void Addtfidf(int sentence, double tfidf) = 0;
  virtual void Print();
  int num_tokens() const {return num_tokens_;}
  bool is_test() const {return is_test_;}
  int id() const {return id_;}
  void set_title(string title);
  const string& title() const;
  Time time_;

 protected:
  bool is_test_;
  int id_;
  int num_tokens_;
  string title_;
};

class ExchDoc: public BaseDoc {
 public:
  ExchDoc(bool is_test, int id);
  void AddWord(int sentence, int word);
  const IntMap& counts() {return counts_;}
  int num_types() {return num_types_;}

 protected:
  IntMap counts_;
  int num_types_;
};

class SeqDoc: public BaseDoc {
 public:
  SeqDoc(bool is_test, int id);
  void AddWord(int sentence, int word);
  void Addtfidf(int sentence, double tfidf);
  virtual void Print();
  uint16_t operator[] (int idx) const;
  double gettfidf(int idx) const;
 private:
  vector<uint16_t> words_;
  vector<double> tfidf_;
};

class MlSeqDoc: public SeqDoc {
 public:
  MlSeqDoc(bool is_test, int id, int language);
  virtual void Print();
  int language() const;
 private:
  int language_;
};

class SemMlSeqDoc : public MlSeqDoc {
 public:
  SemMlSeqDoc(bool is_test, int id, int language);
  void AddWord(int sentence, int word, int synset);
  int synset(int index) const;
 private:
  // This isn't unsigned, as -1 denotes when there is no annotation
  vector<int16_t> synsets_;
};

class ReviewDoc: public MlSeqDoc {
 public:
  ReviewDoc(bool is_test, int id, int language, int author, double rating);
  double rating() const;
  int author() const;
  virtual void Print();
 protected:
  int author_;
  double rating_;
};

class ExchSentDoc : public ExchDoc {
 public:
  ExchSentDoc(bool is_test, int id);
  void AddWord(int sentence, int word);

  const IntMap& sentence_counts(int sentence);

  int num_sentence_types(int sentence);
  int num_sentence_tokens(int sentence);
  int num_sentences() {return num_sentences_;}

 protected:
  vector<IntMap> sentence_counts_;
  vector<uint16_t> num_sentence_tokens_;
  vector<uint16_t> num_sentence_types_;
  int num_sentences_;
};
}

#endif  // TOPICMOD_LIB_CORPORA_DOCUMENT_H_
