/*
 * wordnet.h
 *
 * Copyright 2007, Jordan Boyd-Graber
 *
 * Keeps tracks of counts and distributions over heirarchies.
 */
#ifndef TOPICMOD_PROJECTS_LDAWN_SRC_WORDNET_H__
#define TOPICMOD_PROJECTS_LDAWN_SRC_WORDNET_H__

#include <fstream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>

#include <set>

#include "gsl/gsl_vector.h"

#include "boost/shared_ptr.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/unordered_map.hpp"

#include "../../../lib/corpora/corpus.h"
#include "../../../lib/prob/logspace.h"
#include "../../../lib/prob/multinomial.h"
#include "../../../lib/util/flags.h"

#include "topicmod/projects/ldawn/src/wordnet_file.pb.h"

namespace topicmod_projects_ldawn {

using std::ios;
using boost::shared_ptr;
using lib_corpora::SemMlSeqCorpus;
using lib_corpora::MlSeqCorpus;
using lib_corpora::StrToInt;
using lib_corpora::MlVocab;
using lib_prob::Multinomial;
using lib_prob::SparseMultinomial;
using lib_prob::DegenerateMultinomial;
using lib_prob::DenseMultinomial;
using lib_prob::gsl_blas_dsum;
using lib_prob::gsl_vector_print;

typedef std::set<int> IntSet;
typedef boost::unordered_map<int, int> IntToInt;
typedef boost::unordered_map<int, double> IntToDouble;
typedef boost::unordered_map<int, shared_ptr<Multinomial> > IntToMult;
typedef std::vector< shared_ptr<Multinomial> > MultVector;

const int kTransitionChoice = 1;
const int kEmissionChoice = 0;

bool has_cycle(const int value, const std::vector<int>& visited);

class Synset {
 public:
  Synset(const WordNetFile::Synset& synset, int index, int offset,
         int corpus_id);
  int index() const;
  int offset() const;
  int corpus_id() const;
  int num_children() const;
  int child_index(int ii) const;
  void addChild(int index);
  int num_words(const int lang) const;
  int total_words() const;
  int num_paths() const;
  int term_id(const int lang, const int ii) const;
  int synset_corpus_id() const;

  const string hyperparameter() const;
  double hyponym_count() const;

  void set_depth(int depth);

  friend class WordNet;
  friend class TopicWalk;
  friend void wordnet_init();

  // Replacing old doubles
  double transition_sum() const;
  double emission_sum(int lang) const;
  double choice_sum() const;

 protected:
  void BuildIndex(const WordNetFile::Synset& proto_synset,
                  const vector<StrToInt>& lookup);
  void Finalize();
  void SetPrior(int index, double count);
  boost::shared_ptr<gsl_vector> transition_prior_;
  std::vector< boost::shared_ptr<gsl_vector> > emission_prior_;
  boost::shared_ptr<gsl_vector> choice_prior_;

  std::vector< std::vector<int> > words_;
  std::vector<int> children_indices_;
  bool finalized_;
  string hyperparameter_;
  double hyponym_count_;
  int index_;
  int offset_;
  int num_paths_;
  int depth_;
  int corpus_id_;
};

struct Path {
  // Which index of the word's child array do we use?
  int path_index_;

  // The synsets we traverse.
  std::vector<int> synset_ids_;

  // The offsets of the children in the transition vector of nodes.
  std::vector<int> child_index_;

  // The final word's id in the vocabulary.
  int final_word_id_;

  int final_word_emission_index_;

  int final_word_lang_;

  void Print() const;
};

typedef std::vector<Path> WordPaths;
typedef boost::tuple<double, int, int, int> PathProbTuple;
typedef vector<PathProbTuple> PathProbTupleList;

class WordNet {
 public:
  explicit WordNet();

  // Testing functions.
  friend void wordnet_init();
  friend void topic_walk();
  friend class LdawnState;

  void FindChildrenFromFile(const string filename);
  void AddFile(const string filename, SemMlSeqCorpus* c);
  void FinalizePaths();

  int max_paths() const;
  int num_synsets() const;
  int num_words(int language) const;
  int num_langs() const;

  int index(int offset) const;

  // Returns the offset (not index!) of the root
  int root() const;

  const WordPaths& word(int lang, int ii) const;
  const Synset& synset(int ii) const;
  // Return the final synset of a path
  const Synset& final(int ll, int ww, int path) const;
  const IntSet& debug_synsets() const;

  void BuildPaths();
  void set_debug(bool debug);
  bool debug() const;

 protected:
  const std::vector<Path> empty_path_;
  // The id of the root node
  int SearchDepthFirst(const int depth,
                       const int node_index,
                       vector<int>& traversed,
                       vector<int>& next_pointers);
  int root_;
  int num_synsets_;
  int max_paths_;
  int max_depth_;
  bool debug_;

  // For each language, store the vocabulary size
  std::vector<int> vocab_size_;
  IntToInt offset_to_index_;
  IntSet debug_indices_;
  std::vector<Synset> synsets_;
  std::vector< std::vector<WordPaths> > words_;

  void FindChildrenFromProto(const WordNetFile& wordnet);
  void AddProto(const WordNetFile& wordnet, SemMlSeqCorpus* c);
  void AddDebugPath(const Path& p);
};

class TopicWalk {
 public:
  TopicWalk();

  // For testing
  friend void topic_walk();

  void Initialize(WordNet* wn, double choice, double transition,
                  double emission);
  double LogLikelihood() const;

  // This is not const because we cache probabilities, and that changes the
  // internal state
  double PathProb(const Path& path);
  double SynsetLikelihood(const Synset& synset);
  double WordProb(const WordPaths& word);
  int WordCount(const WordPaths& word);
  void SetHyperparameters(const std::map<string, int>& lookup,
                          const vector< vector <double> >& vals);

  void ChangeCount(const Path& path, const int change);
  void WriteTopWords(int num_to_write,
                     const lib_corpora::MlVocab& vocab,
                     std::ostream* out);
  void WriteTopTransitions(int num_to_write,
                           lib_corpora::MlSeqCorpus* corpus,
                           std::ostream* out);
  void WriteDotTransitions(const string& filename,
                           const IntSet& synsets,
                           lib_corpora::MlSeqCorpus* corpus);
  void FillProbArray(PathProbTupleList* probs);
  void TransitionProbFillArray(PathProbTupleList* probs);

  // number of words in this topic walk
  int sum(int lang) const;
  void Reset();

 protected:
  double TransitionProbability(const int start,
                               const int offset);
  double EmissionProbability(const int synset,
                             const int language,
                             const int word_offset);

  // If a synset transitions or emits
  MultVector dense_choice_;
  IntToMult sparse_choice_;

  // When a synset transitions
  MultVector dense_transition_;
  IntToMult sparse_transition_;

  // When a synset emits
  std::vector<MultVector> dense_emission_;
  std::vector<IntToMult> sparse_emission_;

  IntToDouble cache_;
  WordNet* wordnet_;

  MultVector degenerate_emission_;
  shared_ptr<Multinomial> degenerate_transition_;
  shared_ptr<Multinomial> degenerate_choice_;

  // Get distributions
  Multinomial* choice_dist(int synset);
  Multinomial* transition_dist(int synset);
  Multinomial* emission_dist(int synset, int lang);

  // Static helper functions
  static Multinomial* dist_factory(boost::shared_ptr
                                   <gsl_vector> const & prior,
                                   double scale);
  static Multinomial* duplex_lookup(MultVector* dense_lookup,
                                    IntToMult* sparse_lookup,
                                    bool use_sparse,
                                    int key, Multinomial* default_value);
  static void SetSynsetHyperparameter(Multinomial* m,
                                      double hyperparameter);
};

void toy_proto(WordNetFile* proto);
}

#endif  // TOPICMOD_PROJECTS_LDAWN_SRC_WORDNET_H__
