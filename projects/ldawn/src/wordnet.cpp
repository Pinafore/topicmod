/*
 * wordnet.cpp
 *
 * Copyright 2007, Jordan Boyd-Graber
 *
 * Class to store WordNet hierarchy.
 */

#include <algorithm>
#include <string>
#include <map>
#include <vector>

#include "topicmod/projects/ldawn/src/wordnet.h"
#include "topicmod/lib/corpora/corpus.h"
// #include "math/vectorops.h"

// TODO(JBG): Consider making this a subclass of a distribution, as it
// has many of the same functions as Multinomial

DEFINE_bool(wn_sparse_dist_lookup, true, "Use a sparse lookup for lookups");
DEFINE_bool(wn_sparse_distributions, true,
            "Use a sparse multinomial for wordnet");
DEFINE_bool(wn_use_hyper_sum, false,
            "Treat hyperparameter values as sum, not scale (pseudocount)");
DEFINE_bool(wn_use_cache, false, "Cache synset probabilities");

DEFINE_string(wn_debug_synsets,
              "",
              "Synsets which need to be reported in our debug");

DEFINE_int(wn_min_graphviz_count,
           1,
           "Ignore graphviz nodes with fewer than this many counts");

namespace topicmod_projects_ldawn {

bool has_cycle(const int value, const std::vector<int>& visited) {
  for (int ii = 0; ii < (int)visited.size(); ++ii) {
    if (visited[ii] == value) return true;
  }
  return false;
}

Synset::Synset(const WordNetFile::Synset& synset, int index, int offset,
               int corpus_id) {
  offset_ = offset;
  index_ = index;
  finalized_ = false;
  depth_ = 0;
  num_paths_ = 0;
  corpus_id_ = corpus_id;
  hyponym_count_ = synset.hyponym_count();
  hyperparameter_ = synset.hyperparameter();
}

double Synset::transition_sum() const {
  return gsl_blas_dsum(transition_prior_.get());
}

double Synset::choice_sum() const {
  return gsl_blas_dsum(choice_prior_.get());
}

double Synset::emission_sum(int lang) const {
  return gsl_blas_dsum(emission_prior_[lang].get());
}

int Synset::offset() const {
  return offset_;
}

const string Synset::hyperparameter() const {
  return hyperparameter_;
}

int Synset::corpus_id() const {
  return corpus_id_;
}

int Synset::index() const {
  return index_;
}

int Synset::num_children() const {
  return children_indices_.size();
}

int Synset::child_index(int ii) const {
  BOOST_ASSERT(ii < (int)children_indices_.size());
  return children_indices_[ii];
}

void Synset::addChild(int index) {
  children_indices_.push_back(index);
}

int Synset::total_words() const {
  int val = 0;
  for (int ii = 0; ii < (int)words_.size(); ++ii) val += num_words(ii);
  return val;
}

int Synset::num_words(const int lang) const {
  if (lang < (int)words_.size()) {
    return words_[lang].size();
  } else {
    return 0;
  }
}

int Synset::num_paths() const {
  return num_paths_;
}

int Synset::term_id(const int lang, const int ii) const {
  return words_[lang][ii];
}

void Synset::set_depth(int depth) {
  depth = std::max(depth_, depth);
}

  /*
   *
   */
void Synset::BuildIndex(const WordNetFile::Synset& proto_synset,
                        const vector<StrToInt>& word_lookup) {
  // Figure out the number of words we have
  int num_langs = 0;
  for (int ii = 0; ii < proto_synset.words_size(); ++ii) {
    num_langs = std::max(num_langs,
                         (int)proto_synset.words(ii).lang_id() + 1);
  }

  // Discover all the words and create a temporary record of their
  // prior count.
  std::vector< std::vector<double> > prior_count;

  prior_count.resize(num_langs);
  words_.resize(num_langs);
  emission_prior_.resize(num_langs);
  for (int ii = 0; ii < proto_synset.words_size(); ++ii) {
    int id = -1;
    int lang = proto_synset.words(ii).lang_id();

    // TODO(jbg): There's probably a cleaner way of writing this logic.

    // If this language isn't in our lookup or that word isn't in that
    // languages vocabulary, skip it.
    if (lang >= (int)word_lookup.size()) continue;

    const string& current_word = proto_synset.words(ii).term_str();
    /*
    BOOST_FOREACH(lda::IdLookup::value_type jj, lookup[lang]) {
      cout << jj.first << "," << jj.second;
      if(current_word == jj.first) cout << "*";
      cout << " ";
    }
    */

    StrToInt::const_iterator jj = word_lookup[lang].find(current_word);
    if (jj == word_lookup[lang].end()) {
      // cout << "Term not found in corpus:" << current_word << endl;
      continue;
    } else {
      id = jj->second;
      // cout << "Term found:" << current_word << "=" << jj->second << endl;
    }

    words_[proto_synset.words(ii).lang_id()]
      .push_back(id);
    prior_count[lang]
      .push_back(proto_synset.words(ii).count());
  }

  // Now construct the prior for emission
  for (int ll = 0; ll < num_langs; ++ll) {
    // If there are no words, we don't have to create the priors
    if (words_[ll].size() == 0) continue;

    emission_prior_[ll].reset(gsl_vector_alloc(words_[ll].size()),
                              gsl_vector_free);
    for (int ii = 0; ii < (int)words_[ll].size(); ++ii) {
      gsl_vector_set(emission_prior_[ll].get(), ii,
                     prior_count[ll][ii]);
      // Ensure we have a proper prior.
      assert(prior_count[ll][ii] != 0);
    }
  }

  // Create the prior for deciding whether to emit words or transition
  choice_prior_.reset(gsl_vector_alloc(2),
                      gsl_vector_free);
}

// If we want to have different word and transition priors, this
// will have to change
void Synset::Finalize() {
  finalized_ = true;

  double trans_count = 0.0;
  if (num_children() > 0) {
    trans_count = lib_prob::normalize(transition_prior_.get());
    if (trans_count <= 0) gsl_vector_print(transition_prior_.get());
    assert(trans_count > 0.0);
  }

  double word_count = 0.0;
  for (int ll = 0; ll < (int)words_.size(); ++ll) {
    if (words_[ll].size() > 0) {
      word_count += lib_prob::normalize(emission_prior_[ll].get());
      assert(word_count > 0.0);
    }
  }

  // Take care of the choice prior
  gsl_vector_set(choice_prior_.get(), kTransitionChoice, trans_count);
  gsl_vector_set(choice_prior_.get(), kEmissionChoice, word_count);

  // TODO(jbg): This should actually be strictly greater than 0.0, but
  // we have empty synsets, which wastes memory but isn't (hopefully)
  // a problem.
  assert(lib_prob::normalize(choice_prior_.get()) >= 0);
}

void Synset::SetPrior(int index, double count) {
  if (transition_prior_ == NULL) {
    transition_prior_.reset(gsl_vector_alloc(num_children()),
                            gsl_vector_free);
    gsl_vector_set_all(transition_prior_.get(), 0.0);
  }

  // We can't have a zero count.  Such words should be pruned
  // beforehand.
  assert(count > 0.0 && !isnan(count));
  gsl_vector_set(transition_prior_.get(), index, count);
}

void Path::Print() const {
  assert(synset_ids_.size() == child_index_.size() + 1);
  int ii;
  for (ii = 0; ii < (int)child_index_.size(); ++ii) {
    cout << synset_ids_[ii] << ":" << child_index_[ii] << " ";
  }
  cout << synset_ids_[ii] << " -> " << "(" << final_word_lang_ <<
    "," << final_word_id_ << "," << final_word_emission_index_ <<
    ")" << endl;
}

WordNet::WordNet() {
  num_synsets_ =  0;
  max_depth_   =  0;
  max_paths_   =  0;
  root_        = -1;
  debug_       = false;
}

bool WordNet::debug() const {
  return debug_;
}

void WordNet::set_debug(bool debug) {
  debug_ = debug;
}

int WordNet::root() const {
  assert(root_ > -1);
  return root_;
}


void WordNet::FinalizePaths() {
  // From the user input, add in the debug synsets.
  vector<string> result;
  if (FLAGS_wn_debug_synsets != "") {
    lib_util::SplitStringUsing(FLAGS_wn_debug_synsets, ",", &result);
    for (int ii = 0; ii < (int)result.size(); ++ii) {
      int offset = lib_util::ParseLeadingIntValue(result[ii]);
      if (offset_to_index_.find(offset) != offset_to_index_.end()) {
        int val = offset_to_index_[offset];
        cout << "Adding debug synset: " << offset << "->" << val << endl;
        debug_indices_.insert(val);
      } else {
        cout << "Could not add " << offset << " to debug; offset not found.";
      }
    }
  }

  // Allocate space for paths
  words_.resize(vocab_size_.size());
  for (int ii = 0; ii < (int)vocab_size_.size(); ++ii) {
    words_[ii].resize(vocab_size_[ii]);
  }

  vector<int> traversed;
  vector<int> next;

  assert(root_ >= 0);
  assert(offset_to_index_.find(root_) != offset_to_index_.end());
  max_depth_ = SearchDepthFirst(0, offset_to_index_[root_],
                                traversed, next);

  for (int ll = 0; ll < (int)words_.size(); ++ll) {
    int num_paths = 0;
    for (int ww = 0; ww < (int)words_[ll].size(); ++ww) {
      if (words_[ll][ww].size() > 0) {
        ++num_paths;
      }
    }
    cout << "We found " << num_paths << " paths in language " << ll << endl;
  }
}


int WordNet::max_paths() const {
  return max_paths_;
}

int WordNet::num_synsets() const {
  return synsets_.size();
}


int WordNet::num_words(int language) const {
  return vocab_size_[language];
}


int WordNet::num_langs() const {
  return vocab_size_.size();
}

/*
 * Given the offset of a synset, return its position in the array
 */
int WordNet::index(int offset) const {
  IntToInt::const_iterator ii;
  ii = offset_to_index_.find(offset);
  if (ii == offset_to_index_.end()) {
    return -1;
  } else {
    return ii->second;
  }
}

const WordPaths& WordNet::word(int lang, int ii) const {
  BOOST_ASSERT(empty_path_.size() == 0);
  if (lang >= (int)words_.size() || ii >= (int)words_[lang].size()) {
    return empty_path_;
  }
  return words_[lang][ii];
}

const Synset& WordNet::synset(int ii) const {
  return synsets_[ii];
}

const Synset& WordNet::final(int ll, int ww, int path_index) const {
  BOOST_ASSERT(path_index >= 0);
  BOOST_ASSERT(path_index < (int)word(ll, ww).size());
  const Path& path = word(ll, ww)[path_index];
  int index = path.synset_ids_[path.synset_ids_.size() - 1];
  /*
  cout << "Final synset of word=" << ww << " in lang=" << ll << " with path=" <<
    path_index << " is " << index << endl;
  */
  return synset(index);
}

const IntSet& WordNet::debug_synsets() const {
  return debug_indices_;
}

int WordNet::SearchDepthFirst(const int depth,
                              const int node_index,
                              vector<int>& traversed,
                              vector<int>& next_pointers) {
  int max_depth = depth;
  assert(node_index >= 0);

  // Make sure we don't have a cycle.
  BOOST_ASSERT(!has_cycle(node_index, traversed));
  traversed.push_back(node_index);

  // If we wanted to assert that this were a DAG, we would have a
  // constraint like so:
  // assert(synsets_[node_index].num_paths_==0);
  synsets_[node_index].num_paths_++;

  // cout << "Running depth first seach " << endl;

  // Only add paths that point to words if this synset has words.
  // Yes, we're copying the vectors, and that's what we want to do.
  // Otherwise, the paths would change as we continued our DFS.
  for (int ll = 0; ll < (int)vocab_size_.size(); ++ll) {
     // cout << "Lang " << ll << " ";
     for (int ii = 0; ii < synsets_[node_index].num_words(ll); ++ii) {
      Path p;
      p.synset_ids_ = traversed;
      p.child_index_ = next_pointers;
      p.final_word_id_ = synsets_[node_index].term_id(ll, ii);
      p.final_word_lang_ = ll;
      p.final_word_emission_index_ = ii;

      // p.Print();
      if (node_index == 1 && p.final_word_id_ == 4517) {
          p.Print();
      }
      words_[p.final_word_lang_][p.final_word_id_].push_back(p);
      /*
      cout << "Word " << p.final_word_id_ << " in lang " << 
	p.final_word_lang_ << " now has " << 
	words_[p.final_word_lang_][p.final_word_id_].size() << 
	" paths." << endl;
      */

      if ((int)words_[p.final_word_lang_][p.final_word_id_].size() >
          max_paths_) {
        max_paths_ = words_[p.final_word_lang_][p.final_word_id_].size();
      }

      BOOST_ASSERT(p.child_index_.size() == p.synset_ids_.size() - 1);
      // cout << "List of size:" << p.child_index_.size() << endl;
      // cout << "Adding path "; p.Print();
      // cout << "Max num paths:" << max_paths_ << endl;
    }
    // cout << endl;
  }

  // Now follow that node's children
  for (int ii = 0; ii < synsets_[node_index].num_children(); ++ii) {
    int child_index = synsets_[node_index].child_index(ii);

    next_pointers.push_back(ii);

    // Add information content to the synset.  Note that we could have
    // a different weighting scheme if we used the raw_count().  This
    // doesn't necesarilly make sense, though.
    synsets_[node_index].SetPrior(ii, synsets_[child_index].hyponym_count());


    // If this is in our debug set, we need to add all the parents to
    // the debug set
    if (debug_indices_.find(child_index) != debug_indices_.end()) {
      cout << "Adding parents for debug synset ";
      for (int dd = 0; dd < (int)traversed.size(); ++dd) {
        debug_indices_.insert(traversed[dd]);
        cout << traversed[dd] << " ";
      }
      cout << synsets_[child_index].offset() << endl;
    }

    int child_depth = SearchDepthFirst(depth + 1,
                                       child_index,
                                       traversed,
                                       next_pointers);
    next_pointers.pop_back();
    max_depth = std::max(child_depth, max_depth);
  }

  // Finalize that synset's prior.  If we wanted to change the prior
  // scale based on depth, here would be a good place to implement
  // that.
  synsets_[node_index].Finalize();
  traversed.pop_back();
  return max_depth;
}

double Synset::hyponym_count() const {
  return hyponym_count_;
}

int Synset::synset_corpus_id() const {
  return corpus_id_;
}

void WordNet::FindChildrenFromFile(const string filename) {
  std::fstream input(filename.c_str(), ios::in | ios::binary);
  assert(input);
  WordNetFile proto;
  assert(proto.ParseFromIstream(&input));

  FindChildrenFromProto(proto);
}

void WordNet::AddFile(const string filename, SemMlSeqCorpus* c) {
  std::fstream input(filename.c_str(), ios::in | ios::binary);
  assert(input);
  WordNetFile proto;
  assert(proto.ParseFromIstream(&input));

  AddProto(proto, c);
}

void WordNet::FindChildrenFromProto(const WordNetFile& wordnet) {
  int removed_synsets = 0;
  for (int ii = 0; ii < wordnet.synsets_size(); ++ii) {
    const WordNetFile::Synset& synset = wordnet.synsets(ii);

    for (int jj = 0; jj < synset.children_offsets_size(); ++jj) {
      // If it has children it couldn't be removed
      assert(offset_to_index_.find(synset.offset()) != offset_to_index_.end());
      int parent_index = offset_to_index_[synset.offset()];
      int child_offset = synset.children_offsets(jj);
      if (offset_to_index_.find(child_offset) == offset_to_index_.end()) {
        ++removed_synsets;
      } else {
        synsets_[parent_index].addChild(offset_to_index_[child_offset]);
      }
    }
  }
  cout << removed_synsets << " leaf links" <<
    " were removed because they had no words." << endl;
}

void WordNet::AddProto(const WordNetFile& wordnet, SemMlSeqCorpus* c) {
  if (wordnet.has_root()) {
    int new_root = wordnet.root();
    // Ensure that we haven't previously set the root
    assert(new_root == -1 || root_ == new_root || root_ == -1);
    if (new_root >= 0)
      root_ = new_root;
  }

  // Build a lookup for the vocabulary and synsets
  vector<StrToInt> word_lookup;
  StrToInt synset_lookup;
  c->CreateSynsetVocab(&word_lookup, &synset_lookup);

  int words_added = 0;

  // Add the words to WordNet if they exist in our vocabulary
  for (int ii = 0; ii < wordnet.synsets_size(); ++ii) {
    const WordNetFile::Synset& synset = wordnet.synsets(ii);
    int offset = synset.offset();
    // Make sure that we haven't added this synset before.
    assert(offset_to_index_.find(offset) == offset_to_index_.end());

    for (int jj = 0; jj < synset.words_size(); ++jj) {
      // If we haven't seen this language before, add it to the list
      // of vocab sizes.
      int lang = synset.words(jj).lang_id();
      while ((int)vocab_size_.size() <= lang) {
        vocab_size_.push_back(0);
      }
      BOOST_ASSERT(lang < (int)vocab_size_.size());

      // Now see if the ID assigned by the corpus is less than our
      // current max for the language

      // If this is true, then we are ignoring this language in our vocabulary
      if (lang >= (int)word_lookup.size()) continue;

      StrToInt::const_iterator id =
        word_lookup[lang].find(synset.words(jj).term_str().c_str());

      // The word isn't in our vocab
      if (id == word_lookup[lang].end()) {
        continue;
      } else {
        words_added += 1;
        if (id->second >= vocab_size_[lang]) {
          vocab_size_[lang] = id->second + 1;
        }
      }
    }

    // If we didn't add any words and it has no children in the tree, it's okay
    // if we ignore this synset and don't include it in the tree
    if (words_added == 0 && synset.children_offsets_size() == 0)
      continue;

    int index = synsets_.size();

    // If this synset isn't known to the corpus, add it to the inventory and
    // create a new id
    int corpus_synset_id = c->CheckSynset(synset.key(), &synset_lookup);
    assert(synset_lookup.find(synset.key()) != synset_lookup.end());
    Synset created_synset = Synset(synset, index, offset, corpus_synset_id);
    synsets_.push_back(created_synset);
    synsets_[index].BuildIndex(synset, word_lookup);
    offset_to_index_[offset] = index;

    if (index % 10000 == 0) {
      cout << "Adding synset " << index << " offset=" <<
        synset.offset() << endl;
      for (int jj = 0; jj < synset.children_offsets_size(); ++jj) {
        cout << "  child " << jj << ":" << synset.children_offsets(jj) <<
          endl;
      }
    }
  }
}

void TopicWalk::SetSynsetHyperparameter(Multinomial* m,
                                        double hyperparameter) {
  if (m != NULL) {
    if (FLAGS_wn_use_hyper_sum) {
      m->SetPriorSum(hyperparameter);
    } else {
      m->SetPriorScale(hyperparameter);
    }
  }
}

void TopicWalk::SetHyperparameters(const std::map<string, int>& lookup,
                                   const vector< vector <double> >& vals) {
  int num_syn = wordnet_->num_synsets();
  int num_langs = wordnet_->num_langs();

  for (int ii = 0; ii < num_syn; ++ii) {
    const string& hyp_name = wordnet_->synset(ii).hyperparameter();
    int hyp = 0;
    if (lookup.find(hyp_name) != lookup.end()) {
      hyp = lookup.find(hyp_name)->second;
    }

    double trans = vals[hyp][0];
    double emit = vals[hyp][1];
    double choice = vals[hyp][2];

    if (FLAGS_wn_sparse_dist_lookup) {
      if (sparse_transition_.find(ii) != sparse_transition_.end())
        SetSynsetHyperparameter(sparse_transition_[ii].get(), trans);
      for (int jj = 0; jj < num_langs; ++jj) {
        BOOST_FOREACH(IntToMult::value_type& e, sparse_emission_[jj]) { // NOLINT
          if (sparse_emission_[jj].find(ii) != sparse_emission_[jj].end())
            SetSynsetHyperparameter(e.second.get(), emit);
        }
      }
      if (sparse_choice_.find(ii) != sparse_choice_.end())
        SetSynsetHyperparameter(sparse_choice_[ii].get(), choice);
    } else {
      SetSynsetHyperparameter(dense_transition_[ii].get(), trans);
      BOOST_FOREACH(shared_ptr<Multinomial>& e, dense_emission_[ii])  // NOLINT
        SetSynsetHyperparameter(e.get(), emit);
      SetSynsetHyperparameter(dense_choice_[ii].get(), choice);
    }
  }
}


void TopicWalk::Reset() {
  degenerate_choice_->Reset();
  degenerate_transition_->Reset();
  for (int ll = 0; ll < wordnet_->num_langs(); ++ll)
    degenerate_emission_[ll]->Reset();

  if (FLAGS_wn_sparse_dist_lookup) {
    BOOST_FOREACH(IntToMult::value_type ii, sparse_choice_) ii.second->Reset();
    BOOST_FOREACH(IntToMult::value_type ii, sparse_transition_)
      ii.second->Reset();
    BOOST_FOREACH(IntToMult& lang, sparse_emission_) { // NOLINT
      BOOST_FOREACH(IntToMult::value_type ii, lang) ii.second->Reset();
    }
  } else {
    BOOST_FOREACH(shared_ptr<Multinomial>& c, dense_choice_) {  // NOLINT
      if (c.get() != NULL) c->Reset();
    }
    BOOST_FOREACH(shared_ptr<Multinomial>& t, dense_transition_) { // NOLINT
      if (t.get() != NULL) t->Reset();
    }
    BOOST_FOREACH(MultVector& lang, dense_emission_) { // NOLINT
      BOOST_FOREACH(shared_ptr<Multinomial>& e, lang) {  // NOLINT
        if (e.get() != NULL) e->Reset();
      }
    }
  }
}

int TopicWalk::sum(int lang) const {
  int sum = 0;
  if (FLAGS_wn_sparse_dist_lookup) {
    BOOST_FOREACH(const IntToMult& lang, sparse_emission_) { // NOLINT
      BOOST_FOREACH(IntToMult::value_type ii, lang)
        sum += ii.second->sum();
    }
  } else {
    BOOST_FOREACH(const MultVector& lang, dense_emission_) { // NOLINT
      BOOST_FOREACH(const shared_ptr<Multinomial>& e, lang) { // NOLINT
        if (e.get() != NULL) sum += e->sum();
      }
    }
  }

  BOOST_FOREACH(const shared_ptr<Multinomial>& e, degenerate_emission_) // NOLINT
    sum += e->sum();

  return sum;
}

TopicWalk::TopicWalk() {}

double TopicWalk::TransitionProbability(const int start,
                                        const int offset) {
  double move = transition_dist(start)->LogProbability(offset);
  double choice = 0.0;
  choice = choice_dist(start)->LogProbability(kTransitionChoice);
  if (wordnet_->debug()) {
    cout << "  Trans from " << start << " to child offset " << offset <<
      " M:" << exp(move) << "=" <<
      transition_dist(start)->debug_string(offset) <<
      " C:" << exp(choice) << "=" << choice << endl;
  }
  return move + choice;
}

double TopicWalk::EmissionProbability(int synset_id,
                                      int language,
                                      int word_offset) {
  double emit = emission_dist(synset_id, language)->LogProbability(word_offset);
  // double emit = emission_dist(synset_id, language)
  // ->Delta_LogProb(word_offset);

  double choice = choice_dist(synset_id)->LogProbability(kEmissionChoice);
  if (wordnet_->debug()) {
    cout << "  Emitting from " << synset_id << " in lang " << language <<
      " with word " << word_offset << " E:" << exp(emit) << "=" <<
      emission_dist(synset_id, language)->debug_string(word_offset) << " C:" <<
      exp(choice) << "=" << choice << endl;
  }
  return emit + choice;
}


double TopicWalk::PathProb(const Path& path) {
  // Try to find the synset in the cache
  int ii = 0;
  double val = 0.0;

  if (FLAGS_wn_use_cache) {
    for (ii = path.child_index_.size();
        ii > 0 && cache_.find(path.synset_ids_[ii]) == cache_.end();
        ii--);
    val = cache_[path.synset_ids_[ii]];
    // cout << "CACHED AT " << ii << ", val=" << val << endl;
  }

  // If that fails, we compute from scratch
  for (; ii < (int)path.child_index_.size(); ii++) {
    int synset_id = path.synset_ids_[ii];
    double contribution = TransitionProbability(synset_id,
                                                path.child_index_[ii]);
    // cout << " " << ii << ":" << exp(contribution);
    val += contribution;

    int child_id = path.synset_ids_[ii + 1];
    // The cache only works if there's a unique path to the synset.
    if (FLAGS_wn_use_cache && wordnet_->synset(child_id).num_paths() == 1) {
      cache_[child_id] = val;
    }
  }
  double emit = EmissionProbability(path.synset_ids_[ii],
                                    path.final_word_lang_,
                                    path.final_word_emission_index_);
  val += emit;
  // cout << " emit: " << exp(emit) << endl;
  if (wordnet_->debug()) {
    cout << "Path is ";
    path.Print();
    cout << endl << ",  prob = " << exp(val) << endl;
  }
  return val;
}

double TopicWalk::LogLikelihood() const {
  double val = 0;

  if (FLAGS_wn_sparse_dist_lookup) {
    BOOST_FOREACH(IntToMult::value_type ii, sparse_choice_) {
      if (ii.second != NULL) val += ii.second->LogLikelihood();
    }
    BOOST_FOREACH(IntToMult::value_type ii, sparse_transition_) {
      if (ii.second != NULL) val += ii.second->LogLikelihood();
    }
    BOOST_FOREACH(const IntToMult& lang, sparse_emission_) { // NOLINT
      BOOST_FOREACH(IntToMult::value_type ii, lang) {
        if (ii.second != NULL) val += ii.second->LogLikelihood();
      }
    }
  } else {
    BOOST_FOREACH(const shared_ptr<Multinomial>& c, dense_choice_) {  // NOLINT
      if (c.get() != NULL) val += c->LogLikelihood();
    }
    BOOST_FOREACH(const shared_ptr<Multinomial>& t, dense_transition_) { // NOLINT
      if (t.get() != NULL) val += t->LogLikelihood();
    }
    BOOST_FOREACH(const MultVector& lang, dense_emission_) { // NOLINT
      BOOST_FOREACH(const shared_ptr<Multinomial>& e, lang) {  // NOLINT
        if (e.get() != NULL) val += e->LogLikelihood();
      }
    }
  }

  return val;
}

  /*
   * Can only be called for words that actually have a valid path.
   * Other words must be handled elsewhere (in the parent LDA class).
   */
double TopicWalk::WordProb(const WordPaths& word) {
  assert(word.size() > 0);
  double val = PathProb(word[0]);
  for (int ii = 1; ii < (int)word.size(); ++ii) {
    val = lib_prob::log_sum(val, PathProb(word[ii]));
  }
  return val;
}

Multinomial* TopicWalk::dist_factory(boost::shared_ptr
                                     <gsl_vector> const & prior,
                                     double scale) {
  int length = prior->size;
  double prior_sum = scale;
  if (!FLAGS_wn_use_hyper_sum) prior_sum *= length;
  if (FLAGS_wn_sparse_distributions) {
    SparseMultinomial* m = new SparseMultinomial();
    m->Initialize(length, prior_sum);
    return m;
  } else {
    DenseMultinomial* m = new DenseMultinomial();
    m->Initialize(prior, prior_sum);
    return m;
  }
}

Multinomial* TopicWalk::duplex_lookup(MultVector* dense_lookup,
                                      IntToMult* sparse_lookup,
                                      bool use_sparse, int key,
                                      Multinomial* default_value) {
  if (use_sparse) {
    if (sparse_lookup->find(key) == sparse_lookup->end()) {
      return default_value;
    } else {
      return (*sparse_lookup)[key].get();
    }
  } else {
    if ((*dense_lookup)[key].get() == NULL) {
      return default_value;
    } else {
      return (*dense_lookup)[key].get();
    }
  }
}

Multinomial* TopicWalk::transition_dist(int synset) {
  return duplex_lookup(&dense_transition_, &sparse_transition_,
                       FLAGS_wn_sparse_dist_lookup, synset,
                       degenerate_transition_.get());
}

Multinomial* TopicWalk::choice_dist(int synset) {
  return duplex_lookup(&dense_choice_, &sparse_choice_,
                       FLAGS_wn_sparse_dist_lookup, synset,
                       degenerate_choice_.get());
}

Multinomial* TopicWalk::emission_dist(int synset, int lang) {
  return duplex_lookup(&dense_emission_[lang], &sparse_emission_[lang],
                       FLAGS_wn_sparse_dist_lookup, synset,
                       degenerate_emission_[lang].get());
}

int TopicWalk::WordCount(const WordPaths& word) {
  assert(word.size() > 0);
  int count = 0;
  int lang = word[0].final_word_lang_;
  for (int ii = 0; ii < (int)word.size(); ++ii) {
    int path_length = word[ii].child_index_.size();
    int final_synset = word[ii].synset_ids_[path_length];
    int emission_index = word[ii].final_word_emission_index_;
    count += emission_dist(final_synset, lang)->count(emission_index);
  }
  return count;
}

  /*
   * Caller is responsible for allocating and deleting wordnet
   */
  void TopicWalk::Initialize(WordNet* wn, double choice, double transition,
                             double emission) {
  wordnet_ = wn;

  int num_langs = wn->num_langs();
  int num_synsets = wn->num_synsets();

  // A degenerate multinomial for transitions that have no choice
  degenerate_transition_.reset(new DegenerateMultinomial());
  degenerate_choice_.reset(new DegenerateMultinomial());

  degenerate_emission_.resize(num_langs);
  for (int ii =0; ii < num_langs; ++ii) {
    degenerate_emission_[ii].reset(new DegenerateMultinomial());
  }

  // Size vectors appropriately
  if (FLAGS_wn_sparse_dist_lookup) {
    sparse_emission_.resize(num_langs);
  } else {
    dense_transition_.resize(num_synsets);
    dense_choice_.resize(num_synsets);
    dense_emission_.resize(num_langs);
    for (int ii = 0; ii < num_langs; ++ii)
      dense_emission_[ii].resize(num_synsets);
  }

  for (int ii = 0; ii < num_synsets; ++ii) {
    const Synset& s = wn->synset(ii);

    // Transition distributions
    if (wn->synset(ii).num_children() > 0) {
      Multinomial* m = dist_factory(s.transition_prior_, transition);
      if (FLAGS_wn_sparse_dist_lookup) {
        sparse_transition_[ii].reset(m);
      } else {
        dense_transition_[ii].reset(m);
      }
    }

    // Emission distributions
    for (int ll = 0; ll < wn->num_langs(); ++ll) {
      if (wn->synset(ii).num_words(ll) > 1) {
        Multinomial* m = dist_factory(s.emission_prior_[ll], emission);
        if (FLAGS_wn_sparse_distributions) {
          sparse_emission_[ll][ii].reset(m);
        } else {
          dense_emission_[ll][ii].reset(m);
        }
      }
    }

    // Choice distributions
    if (s.total_words() > 0 && s.num_children() > 0) {
      // There are only two options for this multinomial
      Multinomial* m = dist_factory(s.choice_prior_, choice);
      if (FLAGS_wn_sparse_distributions) {
        sparse_choice_[ii].reset(m);
      } else {
        dense_choice_[ii].reset(m);
      }
    }
  }
}

void TopicWalk::ChangeCount(const Path& path, const int change) {
  if (FLAGS_wn_use_cache) cache_.clear();
  int path_length = path.child_index_.size();
  for (int ii = 0; ii < path_length; ++ii) {
    transition_dist(path.synset_ids_[ii])->ChangeCount(path.child_index_[ii],
                                                       change);
    choice_dist(path.synset_ids_[ii])->ChangeCount(kTransitionChoice, change);
  }
  Multinomial* emit = emission_dist(path.synset_ids_[path_length],
                                    path.final_word_lang_);
  emit->ChangeCount(path.final_word_emission_index_, change);
  // BOOST_ASSERT(emit->sum() == emit->count(path.final_word_emission_index_));
  choice_dist(path.synset_ids_[path_length])->
    ChangeCount(kEmissionChoice, change);
}

void TopicWalk::FillProbArray(PathProbTupleList* probs) {
  int num_langs = wordnet_->num_langs();
  for (int ll = 0; ll < num_langs; ++ll) {
    int num_words = wordnet_->num_words(ll);
    for (int ii = 0; ii < num_words; ++ii) {
      WordPaths wp = wordnet_->word(ll, ii);
      if (wp.size() > 0) {
        probs->push_back(PathProbTuple(WordProb(wp), ii, ll, WordCount(wp)));
      }
    }
  }
}


void TopicWalk::WriteDotTransitions(const string& filename,
                                    const IntSet& synsets,
                                    lib_corpora::MlSeqCorpus* corpus) {
  const lib_corpora::MlVocab& vocab = *corpus->vocab();

  std::ofstream myfile;
  myfile.open(filename.c_str());
  myfile << std::setprecision(4);
  myfile << "digraph states {" << endl;
  myfile << "\tgraph [ colorscheme=\"accent8\" ]" << endl;

  // Colors for coloring word nodes in graphviz output
  const char* kGraphvizColors[6] = {"red", "green", "blue", "gold",
                                    "aquamarine", "darkorchid"};

  int num_langs = wordnet_->num_langs();

  for (IntSet::const_iterator ii = synsets.begin();
       ii != synsets.end(); ii++) {
    // int offset = *ii;
    // int index = wordnet_->index(offset);
    int index = *ii;
    assert(index != -1);
    const Synset& synset = wordnet_->synset(index);

    if (choice_dist(index)->sum() < FLAGS_wn_min_graphviz_count) continue;

    myfile << "\t" << synset.offset() << " [label=\"" <<
      corpus->synset(synset.corpus_id()) << "\"]" << endl;

    for (int ll = 0; ll < num_langs; ++ll) {
      for (int ww = 0; ww < synset.num_words(ll); ++ww) {
        int word = synset.term_id(ll, ww);
        int count = emission_dist(index, ll)->count(ww);
        double prob = exp(EmissionProbability(index, ll, ww));

        // Create a node for the word
        myfile << "\t\t\"" << synset.offset() << "_" << ll << "_" << ww <<
          "\" [label=\"" << vocab[ll][word] <<
          "\",shape=\"box\",color=\"" << kGraphvizColors[ll] << "\"]" <<
          endl;
        // Make an edge connecting
        myfile << "\t\t\t" << synset.offset() << " -> \"" << synset.offset()
               << "_" << ll << "_" << ww << "\" [label=\"" << prob << " (" <<
          count << ")\"]" << endl;
      }
    }

    for (int cc = 0; cc < synset.num_children(); ++cc) {
      int child = synset.child_index(cc);
      if (synsets.find(child) != synsets.end()) {
        myfile << "\t\t" << synset.offset() << " -> " <<
          wordnet_->synset(child).offset() << " [label=\"" <<
          exp(TransitionProbability(index, cc)) << "(" <<
          transition_dist(index)->count(cc) << ")\"]" << endl;
      }
    }
  }
  myfile << "}" << endl;
}

void TopicWalk::TransitionProbFillArray(PathProbTupleList* probs) {
  int root_index = wordnet_->index(wordnet_->root());
  int num_children = wordnet_->synset(root_index).num_children();

  for (int ii = 0; ii < num_children; ++ii) {
    probs->push_back(PathProbTuple(TransitionProbability(root_index, ii), ii,
                                   wordnet_->synset(root_index).child_index(ii),
                                   transition_dist(root_index)->count(ii)));
  }
}

void TopicWalk::WriteTopTransitions(int num_to_write,
                                    lib_corpora::MlSeqCorpus* corpus,
                                    std::ostream* out) {
  PathProbTupleList probs;

  // Get the top root-level transitions
  TransitionProbFillArray(&probs);
  sort(probs.rbegin(), probs.rend());

  if (num_to_write > (int)probs.size())
    num_to_write = probs.size();

  for (int ii = 0; ii < num_to_write; ++ii) {
    int child = boost::get<1>(probs[ii]);
    int index = boost::get<2>(probs[ii]);
    int count = boost::get<3>(probs[ii]);

    if (count > 0)
      (*out) << exp(boost::get<0>(probs[ii])) << " " << child << ":"
             << wordnet_->synset(index).offset() << " " << count <<
        " " << corpus->synset(index) << endl;
  }
}

void TopicWalk::WriteTopWords(int num_to_write,
                              const MlVocab& vocab,
                              std::ostream* out) {
  PathProbTupleList probs;
  FillProbArray(&probs);
  if ((int)probs.size() < num_to_write) {
    num_to_write = probs.size();
  }

  sort(probs.rbegin(), probs.rend());
  for (int ii = 0; ii < num_to_write; ++ii) {
    int id = boost::get<1>(probs[ii]);
    int lang = boost::get<2>(probs[ii]);
    int count = boost::get<3>(probs[ii]);

    if (count > 0)
      (*out) << exp(boost::get<0>(probs[ii])) << " " << lang << ":"
             << id << " " << count << " " << vocab[lang][id] << endl;
  }
}
}
