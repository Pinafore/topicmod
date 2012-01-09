/*
 * state.h
 *
 * Copyright 2009, Jordan Boyd-Graber
 *
 * Stores the state of a sampler for an LDA-style model
 */

#ifndef TOPICMOD_LIB_LDA_STATE_H_
#define TOPICMOD_LIB_LDA_STATE_H_

#include <boost/scoped_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iomanip>

#include "topicmod/lib/prob/multinomial.h"
#include "topicmod/lib/corpora/corpus.h"
#include "topicmod/lib/util/flags.h"
#include "topicmod/lib/corpora/proto/corpus.pb.h"

namespace lib_lda {

const int DEFAULT_LANG = 0;

using std::ios;
using std::cout;
using std::endl;
using std::fstream;
using std::ifstream;
using boost::shared_ptr;
using lib_prob::SparseMultinomial;
using lib_prob::DenseMultinomial;
using lib_prob::VocDist;
using lib_prob::Multinomial;
using lib_corpora::MlVocab;
using lib_corpora_proto::Language;
using lib_corpora_proto::Vocab_Entry;
using lib_corpora_proto::Vocab;
using lib_corpora_proto::Corpus;
using lib_corpora_proto::Document;
using lib_corpora_proto::Document_Time;
using lib_corpora_proto::Document_Sentence;
using lib_corpora_proto::Document_Sentence_Word;

class State {
 protected:
  int num_topics_;

  // A 2D matrix where topic_assignments[i][j] is the topic assignent
  // of the word with offset j in document i.  Is -1 if no topic has been
  // assigned.
  std::vector< std::vector < int > > topic_assignments_;

  std::vector< shared_ptr<Multinomial> > docs_;
  std::vector< shared_ptr<Multinomial> > topics_;

  boost::scoped_ptr<lib_corpora::MlSeqCorpus> corpus_;

  // The filename prefix for output
  string output_;

  virtual void InitializeAssignments(bool random_init);
  void InitializeDocs(double alpha_sum, double alpha_zero);
  virtual void InitializeTopics(double lambda);
  virtual void InitializeOther();

  void WriteAssignments();
  void WriteVerboseAssignments();
  void WriteMalletAssignments();
  virtual void WriteOther();
  virtual void ReadAssignments();
  virtual void ReadOther();
  // Reads the likelihood and returns the current iteration
  int ReadLikelihood();

 public:
  State(int num_topics, string output);
  const string& output_path();

  // Reset the counts, but not the assignments
  virtual void ResetCounts();
  // Initialize the states; caller is responsible for memory
  virtual void Initialize(lib_corpora::MlSeqCorpus* corpus,
                          bool random_init,
                          double alpha_sum,
                          double alpha_zero,
                          double lambda);

  int topic_assignment(int doc, int index) const;

  void WriteHyperparameters(std::vector<double> p);
  std::vector<double> ReadHyperparameters(int dim);
  void SetAlpha(double val);
  void SetLambda(double val);

  virtual double TopicDocProbability(int doc, int topic);
  virtual double TopicVocProbability(int topic, int term);

  void ChangeTopic(int doc, int index, int topic);
  virtual void ChangeTopicCount(int doc, int index,
                                int topic, int change);
  virtual void WriteDocumentTotals();
  virtual void WriteTopicsPreamble(fstream* outfile);
  virtual void WriteTopics(int num_terms);
  virtual void WriteTopicHeader(int topic, fstream* outfile);
  void WriteTopic(int topic, int num_terms,
                  const vector<string>& vocab,
                  fstream* outfile, bool header);

  // Save / Resume a state.
  void WriteState(bool verbose_assignments, bool mallet_assignments);
  int ReadState(int save_delay);

  // Likelihood functions
  double DocLikelihood() const;
  virtual double TopicLikelihood() const;
  virtual void PrintStatus(int iteration) const;
  virtual ~State();
};

class MultilingState : public State {
 public:
  MultilingState(int num_topics, string output);
  virtual void Initialize(lib_corpora::MlSeqCorpus* corpus,
                          bool random_init,
                          double alpha_sum,
                          double alpha_zero,
                          double lambda);
  virtual double TopicVocProbability(int lang, int topic, int term);
  void InitializeTopics(double lambda);
  virtual void ChangeTopicCount(int doc, int index, int topic, int change);
  virtual double TopicLikelihood() const;
  virtual void PrintStatus(int iteration) const;
  void ResetCounts();
  void SetAlpha(double val);

 protected:
  // A 2D matrix where topic_counts_[l][i][j] is the number of times
  // instances of word j has been assigned to topic i in language l
  std::vector< std::vector < VocDist > > multilingual_topics_;
  void WriteTopics(int num_terms);
  void WriteTopicsPreamble(fstream* outfile);
  virtual void WriteMultilingualTopic(int topic, int num_terms,
                                      fstream* outfile);

  int num_langs_;
  int vocab_limit_;
};
}
#endif  // TOPICMOD_LIB_LDA_STATE_H_
