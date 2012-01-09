/*
 * Copyright 2009 Author: Jordan Boyd-Graber
 *
 * This is a file to read document information from protocol buffers into a
 * corpus object.
 *
 * TODO(jbg): There is no reader just to create a plain vanilla corpus (even
 * MlSeq).  This should be added and then the regular lda sampler could use it.
 */

#ifndef TOPICMOD_LIB_CORPORA_CORPUS_READER_H_
#define TOPICMOD_LIB_CORPORA_CORPUS_READER_H_

#include <map>
#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include "topicmod/lib/corpora/corpus.h"
#include "topicmod/lib/corpora/proto/corpus.pb.h"

using std::ios;
using std::cout;
using std::endl;
using std::map;
using std::set;
using std::vector;
using std::string;
using std::fstream;
using lib_corpora_proto::Document_Sentence_Word;
using lib_corpora_proto::Vocab;

// Forward declaration of test function
namespace lib_test {
  void test_vocab_map();
}

typedef map<int, int> IntIntMap;
typedef map<string, int> StringIntMap;
typedef lib_corpora_proto::Document proto_doc;

namespace lib_corpora {

void ReadVocab(const string& filename, vector< vector<string> >* vocab);

class CorpusReader {
 public:
  CorpusReader(bool use_lemma, bool use_bigram, bool remove_stop,
               int min_words, int train_limit, int test_limit);
  void ReadProtobuf(const string& location, const string& filename,
                    bool is_test, const vector< vector<string> >& vocab,
                    const vector< vector<string> >& pos, Corpus* corpus);
  virtual ~CorpusReader() {}
 protected:
  void FindValidPos(const lib_corpora_proto::Corpus& corpus,
                    const vector< vector<string> >& filter_pos,
                    vector< set<int> >* valid_pos);

  void CreateVocabMap(const Vocab& corpus_vocab,
                      const vector< vector<string> >& filter_vocab,
                      vector<IntIntMap>* lookup);
  void CreateVocabMaps(const lib_corpora_proto::Corpus& corpus,
                       const vector< vector<string> >& filter_vocab,
                       vector<IntIntMap>* lookup);
  static void CreateUnfilteredMap(const lib_corpora_proto::Vocab& synset_voc,
                                  StringIntMap* lookup, IntIntMap* mapping);
  static void CreateFilteredMap(const lib_corpora_proto::Vocab& corpus_vocab,
                                const vector<string>& filter_vocab,
                                IntIntMap* lookup);
  virtual BaseDoc* DocumentFactory(const proto_doc& source,
                                   const IntIntMap& vocab,
                                   const set<int>& pos,
                                   const IntIntMap& authors,
                                   const IntIntMap& synsets,
                                   bool is_test) = 0;
  virtual void FillDocument(const proto_doc& source, const IntIntMap& vocab,
                            const set<int>& pos, const IntIntMap& synsets,
                            BaseDoc* doc);
  virtual bool AddWord(const lib_corpora_proto::Document_Sentence_Word& word,
                       int sentence, const IntIntMap& vocab,
                       const set<int>& pos, const IntIntMap& synsets,
                       BaseDoc* doc);
  virtual bool is_valid_doc(BaseDoc* doc);
  virtual void AddDocument(Corpus* corpus, BaseDoc* doc);

  bool use_lemma_;
  bool use_bigram_;
  bool remove_stop_;
  int min_words_;
  int test_limit_;
  int train_limit_;
  map<string, int> synset_lookup_;
  map<string, int> author_lookup_;
 private:
  friend void lib_test::test_vocab_map();
};

class ReviewReader: public CorpusReader {
 public:
  ReviewReader(bool use_lemma, bool use_bigram, bool remove_stop, int min_words,
               int train_limit, int test_limit, int rating_train_limit,
               int rating_test_limit, const set<double>& filter_ratings);
 protected:
  BaseDoc* DocumentFactory(const proto_doc& source,
                           const IntIntMap& vocab,
                           const set<int>& pos,
                           const IntIntMap& authors,
                           const IntIntMap& synsets,
                           bool is_test);
  virtual bool is_valid_doc(BaseDoc* doc);
  virtual void AddDocument(Corpus* corpus, BaseDoc* doc);
  const set<double> filter_ratings_;
  int rating_train_limit_;
  int rating_test_limit_;
  std::map<double, int> rating_test_count_;
  std::map<double, int> rating_train_count_;
};

class SemanticCorpusReader : public CorpusReader {
 public:
  SemanticCorpusReader(bool use_lemma, bool use_bigram, bool remove_stop,
                       int min_words, int train_limit, int test_limit);
 protected:
  BaseDoc* DocumentFactory(const proto_doc& source,
                           const IntIntMap& vocab,
                           const set<int>& pos,
                           const IntIntMap& authors,
                           const IntIntMap& synsets,
                           bool is_test);
  virtual bool AddWord(const lib_corpora_proto::Document_Sentence_Word& word,
                       int sentence, const IntIntMap& vocab,
                       const set<int>& pos, const IntIntMap& synsets,
                       BaseDoc* doc);
};
}
#endif  // TOPICMOD_LIB_CORPORA_CORPUS_READER_H_
