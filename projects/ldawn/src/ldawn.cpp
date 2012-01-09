/*
 * Ldawn.cpp
 *
 * Copyright 2007, Jordan Boyd-Graber
 *
 * Class to implement a topic model with a ontological topic distribution over
 * words.
 */

#include <map>
#include <string>
#include <vector>

#include "topicmod/projects/ldawn/src/ldawn.h"

DEFINE_bool(use_aux_topics,
            true,
            "Use special multinomial distributions for terms that don't appear"
            "in the ontology.");

DEFINE_int(level_report, -1, "Write summary of document ontology usage"
           "for level");

DEFINE_string(wn_hyperparam_file,
              "",
              "Where we read default hyperparmeter values for walk");

DEFINE_double(transition_prior, 1.0, "Transition prior for WNWalk");
DEFINE_double(emission_prior, 1.0, "Emission prior for WNwalk");
DEFINE_double(choice_prior, 1.0, "Choice prior for WNwalk");

namespace topicmod_projects_ldawn {

Ldawn::Ldawn(double alpha, double alpha_zero, double lambda,
             int num_topics, string wn_location)
  : MultilingSampler(alpha, alpha_zero, lambda, num_topics) {
  wordnet_location_ = wn_location;

  // TODO(jbg): It's bad to have file IO in a constructor.  This should be
  // factored out into an initialize method, but that would require changing the
  // calling code.

  hyperparameter_names_.push_back("");
  hyperparameter_lookup_[""] = 0;

  if (FLAGS_wn_hyperparam_file != "") {
    hyperparameter_file_ = true;
    std::fstream hyper_file((FLAGS_wn_hyperparam_file).c_str(), ios::in);
    int num_hyperparameters;
    hyper_file >> num_hyperparameters;

    string name;
    double trans, emission, choice;
    hyperparameter_values_.resize(num_hyperparameters + 1);
    for (int ii = 0; ii < num_hyperparameters; ++ii) {
      hyper_file >> name;
      hyper_file >> trans;
      hyper_file >> emission;
      hyper_file >> choice;

      if (hyperparameter_lookup_.find(name) != hyperparameter_lookup_.end()) {
        std::cerr << "Hyperparamter " << name <<
          " appears twice in specification";
      }
      assert(hyperparameter_lookup_.find(name) ==
             hyperparameter_lookup_.end());

      hyperparameter_names_.push_back(name);
      hyperparameter_lookup_[name] = ii + 1;

      hyperparameter_values_[ii + 1].resize(3);
      hyperparameter_values_[ii + 1][0] = trans;
      hyperparameter_values_[ii + 1][1] = emission;
      hyperparameter_values_[ii + 1][2] = choice;
    }
  } else {
    hyperparameter_values_.resize(1);
  }

  hyperparameter_values_[0].resize(3);

  // Insert default values
  hyperparameter_values_[0][0] = FLAGS_transition_prior;
  hyperparameter_values_[0][1] = FLAGS_emission_prior;
  hyperparameter_values_[0][2] = FLAGS_choice_prior;
}

LdawnState* Ldawn::state() {
  return static_cast<LdawnState*>(state_.get());
}

bool Ldawn::use_aux_topics() {
  return FLAGS_use_aux_topics;
}

void Ldawn::ResetTest(int doc) {
  MlSeqDoc* d = corpus_->seq_doc(doc);
  int lang = d->language();
  LdawnState* state = static_cast<LdawnState*> (state_.get());

  int num_words = corpus_->seq_doc(doc)->num_tokens();
  for (int ww = 0; ww < num_words; ++ww) {
    int term = (*d)[ww];
    const WordPaths word = state->wordnet()->word(lang, term);
    int num_paths = word.size();

    if (num_paths == 0) {
      if (FLAGS_use_aux_topics)
        state->ChangeTopic(doc, ww, -1);
    } else {
      state->ChangePath(doc, ww, -1, -1);
    }
  }
}

void Ldawn::InitializeState(const string& output_location, bool random_init) {
  cout << "Creating new LDAWN State" << endl;
  state_.reset(new LdawnState(num_topics_, output_location, wordnet_location_));
  state_->Initialize(corpus_, random_init, alpha_sum_, alpha_zero_, lambda_);
}

void Ldawn::InitializeTemp() {
  int max_paths = state()->wordnet()->max_paths();
  assert(max_paths > 0);
  temp_probs_.resize(max_paths * num_topics_);
}

double Ldawn::LogProbIncrement(int doc, int index) {
  MlSeqDoc* d = corpus_->seq_doc(doc);
  int term = (*d)[index];
  int lang = d->language();
  LdawnState* state = static_cast<LdawnState*> (state_.get());
  const WordPaths word = state->wordnet()->word(lang, term);
  int num_paths = word.size();

  // Check to see how many paths it has.  If doesn't have any, then we
  // ignore it.
  if (num_paths == 0) {
    if (FLAGS_use_aux_topics) {
      return MultilingSampler::LogProbIncrement(doc, index);
    } else {
      // Or ignore it
      return 0.0;
    }
  } else {
    int topic = state->topic_assignment(doc, index);
    int path = state->path_assignment(doc, index);
    double val = state->TopicDocProbability(doc, topic);
    // cout << "Topic alone : " << exp(val);
    val += state->topic_walks_[topic].PathProb(word[path]);
    // cout << " with path : " << exp(val) << endl;
    state->ChangePathCount(doc, index, topic, path, +1);
    return val;
  }
}

vector<string> Ldawn::hyperparameter_names() {
  vector<string> names;
  names.push_back("iter");
  names.push_back("alpha");
  names.push_back("lambda");

  for (int ii = 0; ii < (int)hyperparameter_names_.size(); ++ii) {
    names.push_back("WN_HYP:" + hyperparameter_names_[ii] + ":TRANS");
    names.push_back("WN_HYP:" + hyperparameter_names_[ii] + ":EMIT");
    names.push_back("WN_HYP:" + hyperparameter_names_[ii] + ":CHOICE");
  }

  return names;
}

vector<double> Ldawn::hyperparameters() {
  int hyperparameter_size = HYP_VECTOR_OFFSET +
    HYP_VECTOR_STEP * hyperparameter_values_.size();
  vector<double> p(hyperparameter_size);

  p[0] = log(alpha_sum_);
  p[1] = log(lambda_);

  int index = HYP_VECTOR_OFFSET;
  for (int ii = 0; ii < (int)hyperparameter_values_.size(); ++ii) {
    for (int jj = 0; jj < HYP_VECTOR_STEP; ++jj) {
      p[index++] = log(hyperparameter_values_[ii][jj]);
    }
  }
  assert(index == hyperparameter_size);

  cout << "Populated hyperparamters: " << endl;
  for (int ii = 0; ii < p.size(); ++ii) cout << p[ii] << " ";
  cout << endl;

  return p;
}

double Ldawn::WriteLhoodUpdate(std::ostream* lhood_file, int iteration,
                               double elapsed) {
  // double lhood = TrainLikelihoodIncrement();
  double lhood = Likelihood();
  double test = TestLikelihoodIncrement();
  LdawnState* s = static_cast<LdawnState*> (state_.get());

  (*lhood_file) << iteration;
  (*lhood_file) << "\t" << lhood;
  (*lhood_file) << "\t" << test;
  (*lhood_file) << "\t" << s->WriteDisambiguation(iteration);
  (*lhood_file) << "\t" << elapsed;
  return lhood;
}

void Ldawn::SetHyperparameters(const vector<double>& hyperparameters) {
  set_alpha(exp(hyperparameters[0]));
  set_lambda(exp(hyperparameters[1]));
  LdawnState* s = static_cast<LdawnState*> (state_.get());

  for (int ii = 0; ii < (int)hyperparameter_values_.size(); ++ii) {
    for (int jj = 0; jj < HYP_VECTOR_STEP; ++jj) {
      hyperparameter_values_[ii][jj] =
        exp(hyperparameters[HYP_VECTOR_OFFSET + ii * HYP_VECTOR_STEP + jj]);
    }
  }

  s->SetWalkHyperparameters(hyperparameter_lookup_,
                            hyperparameter_values_);
}

int Ldawn::SampleWordTopic(int doc, int index) {
  MlSeqDoc* d = corpus_->seq_doc(doc);
  int term = (*d)[index];
  int lang = d->language();
  LdawnState* s = static_cast<LdawnState*> (state_.get());
  const WordPaths word = s->wordnet()->word(lang, term);
  int num_paths = word.size();

  // Check to see how many paths it has.  If doesn't have any, then we
  // ignore it.
  if (num_paths == 0) {
    if (FLAGS_use_aux_topics) {
      // If there aren't any paths, then just treat it like a
      // multilingual topic model.
      return MultilingSampler::SampleWordTopic(doc, index);
    } else {
      // Or ignore it
      state_->ChangeTopic(doc, index, -1);
      return -1;
    }
  } else {
    // First, decrement the count if it's assigned
    static_cast<LdawnState*> (state_.get())->ChangePath(doc, index, -1, -1);

    #pragma omp parallel num_threads(num_threads_)
    #pragma omp for schedule (static)
    for (int kk = 0; kk < num_topics_; ++kk) {
      // Probability of that topic being selected.
      double doc_contribution = state_->TopicDocProbability(doc, kk);
      int index = kk * num_paths;

      for (int pp = 0; pp < num_paths; ++pp) {
        BOOST_ASSERT(index < (int)temp_probs_.size());
        BOOST_ASSERT(index / num_paths == kk);
        BOOST_ASSERT(index % num_paths == pp);
        temp_probs_[index] = doc_contribution;
        // Probability of that path being chosen
        temp_probs_[index] += static_cast<LdawnState*>
          (state_.get())->topic_walks_[kk].PathProb(word[pp]);
        BOOST_ASSERT(!isnan(temp_probs_[index]));
        ++index;
      }
      BOOST_ASSERT(index = (kk + 1) * num_topics_);
    }

/*
    // yn added for testing
    const lib_corpora::MlSeqDoc* d = corpus_->seq_doc(doc);
    int term = (*d)[index];
    if (term == 47) {
      int cur = 0;
      int other = 1;
      for (int ii = 0; ii < num_topics_; ++ii) {
        int index = ii * num_paths;
        if (record_[cur][ii] > 0 && record_[other][ii] > 0) {
          BOOST_ASSERT(1 < 0);
        } else if (record_[cur][ii] > 0 && record_[other][ii] == 0) {
          for (int pp = 0; pp < num_paths; ++pp) {
            temp_probs_[index] = 0.0;
            ++index;
          }
        } else if (record_[cur][ii] == 0 && record_[other][ii] > 0) {
          for (int pp = 0; pp < num_paths; ++pp) {
            temp_probs_[index] = -1e99;
            ++index;
          }
        } else {
          for (int pp = 0; pp < num_paths; ++pp) {
            temp_probs_[index] = log(1.0 / num_topics_);
            ++index;
          }
        }
      }
    } else if (term == 144) {
      int cur = 1;
      int other = 0;
      for (int ii = 0; ii < num_topics_; ++ii) {
        int index = ii * num_paths;
        if (record_[cur][ii] > 0 && record_[other][ii] > 0) {
          BOOST_ASSERT(1 < 0);
        } else if (record_[cur][ii] > 0 && record_[other][ii] == 0) {
          for (int pp = 0; pp < num_paths; ++pp) {
            temp_probs_[index] = 0.0;
            ++index;
          }
        } else if (record_[cur][ii] == 0 && record_[other][ii] > 0) {
          for (int pp = 0; pp < num_paths; ++pp) {
            temp_probs_[index] = -1e99;
            ++index;
          }
        } else {
          for (int pp = 0; pp < num_paths; ++pp) {
            temp_probs_[index] = log(1.0 / num_topics_);
            ++index;
          }
        }
      }
    }
    // yn_test ends
*/

    BOOST_ASSERT(num_topics_ * num_paths <= (int)temp_probs_.size());

    int sampled_index = lib_prob::
      log_vector_sample(temp_probs_, num_topics_ * num_paths);
    int new_topic = sampled_index / num_paths;
    int new_path = sampled_index % num_paths;

    /*
    lib_corpora::MlVocab* ml_vocab = corpus_->vocab();
    const lib_corpora::Lookup vocab = (*ml_vocab)[lang];
    const string word_string = vocab[word[new_path].final_word_id_];
    cout << "Word " << word[new_path].final_word_id_ << " " <<
      word_string << " had " << num_paths << "paths." << endl;
    */

    BOOST_ASSERT(new_topic < num_topics_);
    static_cast<LdawnState*> (state_.get())->ChangePath(doc, index,
                                                        new_topic, new_path);
    return new_topic;
  }
}

  /*

Deprecated, never seemed to give the right answer anyway

double Ldawn::Likelihood() {
  double val;
  if (FLAGS_use_aux_topics) {
    val = MultilingSampler::Likelihood();
  } else {
    val = state_->DocLikelihood();
  }

  for (int kk = 0; kk < num_topics_; ++kk) {
    val += static_cast<LdawnState*>
      (state_.get())->topic_walks_[kk].Likelihood();
  }

  return val;
}
  */

void LdawnState::WriteDocumentTotals() {
  if (FLAGS_level_report > 0) {
    cout << "Writing level information to " << output_ + ".doc_levels" << endl;
    fstream outfile((output_ + ".doc_levels").c_str(), ios::out);

    IntMap counts;
    vector<IntMap> topic_counts;
    topic_counts.resize(num_topics_);

    for (int ii = 0; ii < (int)topic_assignments_.size(); ii++) {
      counts.clear();
      int num_words = topic_assignments_[ii].size();
      int lang = corpus_->seq_doc(ii)->language();

      for (int jj = 0; jj < num_words; ++jj) {
        int term = (*corpus_->seq_doc(ii))[jj];
        int path_index = path_assignments_[ii][jj];
        int topic_index = topic_assignments_[ii][jj];

        if (path_index < 0) continue;
        const Path& path = wordnet_->word(lang, term)[path_index];
        if (FLAGS_level_report < (int)path.synset_ids_.size()) {
          int synset = path.synset_ids_[FLAGS_level_report];
          counts[synset] += 1;
          topic_counts[topic_index][synset] += 1;
        }
      }

      BOOST_FOREACH(IntMap::value_type ii, counts) {
        int id = ii.first;
        int count = ii.second;

        outfile << wordnet_->synset(id).offset() << ":" << count << " ";
      }
      outfile << endl;
    }
    outfile.close();

    outfile.open((output_ + ".topic_levels").c_str(), ios::out);
    for (int kk = 0; kk < num_topics_; ++kk) {
      BOOST_FOREACH(IntMap::value_type ii, topic_counts[kk]) {
        int id = ii.first;
        int count = ii.second;

        outfile << wordnet_->synset(id).offset() << ":" << count << " ";
      }
      outfile << endl;
    }
  }

  State::WriteDocumentTotals();
}

const WordNet* LdawnState::wordnet() {
  return wordnet_.get();
}

bool LdawnState::use_aux_topics() {
  return FLAGS_use_aux_topics;
}

double LdawnState::TopicLikelihood() const {
  double val = 0;
  if (FLAGS_use_aux_topics) {
    val += MultilingState::TopicLikelihood();
  }
  BOOST_FOREACH(const TopicWalk& tt, topic_walks_)  // NOLINT
    val += tt.LogLikelihood();

  return val;
}

int LdawnState::path_assignment(int doc, int index) const {
  BOOST_ASSERT(doc >= 0);
  BOOST_ASSERT(index >= 0);
  return path_assignments_[doc][index];
}

LdawnState::LdawnState(int num_topics, string output, string wn_location)
  : MultilingState(num_topics, output) {
  wordnet_location_ = wn_location;
}

void LdawnState::SetWalkHyperparameters(const std::map<string, int>& lookup,
                                        const vector< vector <double> >& vals) {
  BOOST_FOREACH(TopicWalk& tt, topic_walks_)  // NOLINT
    tt.SetHyperparameters(lookup, vals);
}

void LdawnState::ResetCounts() {
  BOOST_FOREACH(TopicWalk& tt, topic_walks_) tt.Reset(); // NOLINT
  MultilingState::ResetCounts();
}

void LdawnState::ChangePathCount(int doc, int index, int topic,
                                 int path, int change) {
  BOOST_ASSERT(topic >= 0 && path >= 0);
  MlSeqDoc* d = corpus_->seq_doc(doc);
  int term = (*d)[index];
  int lang = d->language();

  const WordPaths w = wordnet_->word(lang, term);
  BOOST_ASSERT(path == -1 || path < (int)w.size());

  docs_[doc]->ChangeCount(topic, change);
  topic_walks_[topic].ChangeCount(w[path], change);
}

void LdawnState::ChangePath(int doc, int index, int topic, int path) {
  int current_path = path_assignments_[doc][index];
  int current_topic = topic_assignments_[doc][index];

  if (current_topic >= 0) {
    // It's possible that the topic is set but the path is not if
    // we're initializing the state.
    ChangePathCount(doc, index, current_topic, current_path, -1);
  }

  topic_assignments_[doc][index] = topic;
  path_assignments_[doc][index] = path;

  if (topic >= 0) {
    ChangePathCount(doc, index, topic, path, +1);
  }
}

void LdawnState::PrintStatus(int iteration) const {
  for (int ii = 0; ii < num_langs_; ++ii) {
    int sum_tree_assigned = 0;
    int sum_flat_assigned = 0;
    int topics_used = 0;
    for (int jj = 0; jj < num_topics_; ++jj) {
      int tree_assigned = topic_walks_[jj].sum(ii);
      int flat_assigned = multilingual_topics_[ii][jj].sum();
      sum_flat_assigned += flat_assigned;
      sum_tree_assigned += tree_assigned;
      int topic_usage = tree_assigned + flat_assigned;
      if (topic_usage > 0) topics_used+=1;
    }
    cout << "Language " << ii << " used " << sum_tree_assigned <<
      " tree words and " << sum_flat_assigned << " flat words in " <<
      topics_used << " topics." << endl;
  }
}

void LdawnState::WriteMultilingualTopic(int topic, int num_terms,
                                        std::fstream* outfile) {
  WriteTopicHeader(topic, outfile);
  (*outfile) << "Ontology multinomial " << "(";
  for (int ll = 0; ll < num_langs_; ++ll) {
    (*outfile) << "Lang " << ll << ":" << topic_walks_[topic].sum(ll);
    if (ll < num_langs_ - 1) (*outfile) << ", ";
  }
  (*outfile) << ") :" << endl;
  lib_corpora::MlVocab* vocab = corpus_->vocab();
  topic_walks_[topic].WriteTopWords(num_terms, *vocab, outfile);

  (*outfile) << "   > Top ontology terms <" << endl;
  topic_walks_[topic].WriteTopTransitions(num_terms, corpus_.get(), outfile);

  for (int ll = 0; ll < num_langs_; ++ll) {
    (*outfile) << "          *** Lang " << ll << " (" <<
      multilingual_topics_[ll][topic].sum() << ") ***" << endl;
    (*outfile) << "Non-ontology multinomial:" << endl;
    multilingual_topics_[ll][topic].
      WriteTopWords(num_terms, (*vocab)[ll], outfile);
  }
}

void LdawnState::InitializeAssignments(bool random_init) {
  int num_documents = corpus_->num_docs();
  path_assignments_.resize(num_documents);
  topic_assignments_.resize(num_documents);

  cout << "Initializing path and topic assignments in " <<
    num_documents << " docs  ... " << endl;

  int npaths = 0;
  for (int ii = 0; ii < num_documents; ii++) {
    int num_words = corpus_->doc(ii)->num_tokens();
    MlSeqDoc* d = corpus_->seq_doc(ii);
    int lang = d->language();

    path_assignments_[ii].resize(num_words);
    topic_assignments_[ii].resize(num_words);

    for (int jj = 0; jj < num_words; ++jj) {
      int term = (*d)[jj];
      const WordPaths word = wordnet_->word(lang, term);

      topic_assignments_[ii][jj] = -1;
      path_assignments_[ii][jj] = -1;

      int num_senses = word.size();
      int topic = rand() % num_topics_;

      int path = -1;
      if (num_senses != 0)
        path = rand() % num_senses;

      if (random_init) {
        if (num_senses == 0) {
          if (FLAGS_use_aux_topics)
            ChangeTopic(ii, jj, topic);
        } else {
          ChangePath(ii, jj, topic, path);
        }
      }
      ++npaths;
    }
  }
  cout << "     done on " << npaths << " tokens." << endl;
}

void LdawnState::InitializeWordNet() {
  bool toy = (wordnet_location_ == "");
  cout << "Initializing WordNet (toy=" << toy << ", file=" <<
    wordnet_location_ << ")" << endl;
  wordnet_.reset(new WordNet());
  SemMlSeqCorpus* corpus = static_cast<SemMlSeqCorpus*>(corpus_.get());
  if (toy) {
    WordNetFile proto;
    toy_proto(&proto);
    wordnet_->set_debug(true);
    wordnet_->AddProto(proto, corpus);
    wordnet_->FindChildrenFromProto(proto);
  } else {
    std::vector<string> wn_files;
    lib_util::SplitStringUsing(wordnet_location_, ",", &wn_files);
    assert(wn_files.size() > 0);
    for (int ii = 0; ii < (int)wn_files.size(); ++ii) {
      wordnet_->AddFile(wn_files[ii], corpus);
    }
    // Now that we know about all the synsets, find the children
    for (int ii = 0; ii < (int)wn_files.size(); ++ii) {
      wordnet_->FindChildrenFromFile(wn_files[ii]);
    }
  }
  cout << "Done reading wordnet with root " << wordnet_->root() <<
    ", now finalizing paths." << endl;
  wordnet_->FinalizePaths();

  topic_walks_.resize(num_topics_);
  for (int kk = 0; kk < num_topics_; ++kk) {
    if (kk % 50 == 0) cout << "Initialized topic " << kk << endl;
    topic_walks_[kk].Initialize(wordnet_.get(), FLAGS_choice_prior,
                                FLAGS_transition_prior, FLAGS_emission_prior);
  }
}

void LdawnState::InitializeWsdAnswer() {
  // Open up a place to store whether each document has answers
  cout << "Initializing answer cache." << endl;

  doc_has_answer_.resize(corpus_->num_docs());

  for (int ii = 0; ii < (int)corpus_->num_docs(); ii++) {
    doc_has_answer_[ii] = false;
    const SemMlSeqDoc* d = static_cast<SemMlSeqDoc*>(corpus_->seq_doc(ii));
    int num_words = d->num_tokens();
    for (int jj = 0; jj < num_words; ++jj) {
      int answer = d->synset(jj);
      // cout << ii << " " << jj << " synset:" << answer << endl;
      if (answer != -1) {
        // cout << ii << " has answer" << endl;
        doc_has_answer_[ii] = true;
      }
    }
  }
}

void LdawnState::InitializeOther() {
  InitializeWordNet();
  InitializeWsdAnswer();
}

void LdawnState::ReadAssignments() {
  cout << "Reading assignments from " << output_ + ".topic_assignments" <<
    endl;
  cout << "Reading paths from " << output_ + ".path_assignments" <<
    endl;

  std::fstream topic_file((output_ + ".topic_assignments").c_str(), ios::in);
  std::fstream path_file((output_ + ".path_assignments").c_str(), ios::in);

  int topic;
  int path;
  int num_words;

  for (int ii = 0; ii < (int)topic_assignments_.size(); ii++) {
    assert(topic_file >> num_words);
    assert(num_words == (int)topic_assignments_[ii].size());
    assert(path_file >> num_words);
    assert(num_words == (int)topic_assignments_[ii].size());
    for (int jj = 0; jj < num_words; ++jj) {
      assert(topic_file >> topic);
      assert(path_file >> path);

      if (path >= 0) {
        ChangePath(ii, jj, topic, path);
      } else {
        if (topic >= 0) {
          BOOST_ASSERT(wordnet_->
                       word(corpus_->seq_doc(ii)->language(),
                            (*corpus_->seq_doc(ii))[jj])
                       .size() == 0);
          BOOST_ASSERT(FLAGS_use_aux_topics);
          ChangeTopic(ii, jj, topic);
        }
      }
    }
  }
}

double LdawnState::WriteDisambiguation(int iteration) const {
  std::ofstream accuracy_file;

  // If no document has any answer, just return
  bool any_answers = false;
  for (int ii = 0; ii < corpus_->num_docs(); ii++) {
    any_answers = any_answers || doc_has_answer_[ii];
  }

  if (!any_answers) return -1.0;

  // TODO(jbg): Move this code to where we write to lhood_file
  // Open the file for accuracy information
  if (iteration == 0) {
    accuracy_file.open((output_ + ".acc").c_str(), ios::out);
  } else {
    accuracy_file.open((output_ + ".acc").c_str(), ios::app);
  }

  // Write the disambiguation state and record the accuracy
  std::fstream outfile((output_ + ".synsets").c_str(), ios::out);

  int right = 0;
  int wrong = 0;

  for (int ii = 0; ii < corpus_->num_docs(); ii++) {
    outfile << "      DOC " << ii << endl;

    // Skip documents without answers
    if (!doc_has_answer_[ii]) continue;
    const SemMlSeqDoc* d = static_cast<SemMlSeqDoc*>(corpus_->seq_doc(ii));
    int lang = d->language();
    for (int jj = 0; jj < (int)path_assignments_[ii].size(); ++jj) {
      int word = (*d)[jj];
      int answer_index = d->synset(jj);

      // If there are two paths that end in the same synset, this
      // counts that as a correct answer when it's not
      if (answer_index == -1 ||
          path_assignments_[ii][jj] == -1 ||
          wordnet_->word(lang, word).size() < 1) continue;
      const string& answer = static_cast<lib_corpora::SemMlSeqCorpus*>
        (corpus_.get())->synset(answer_index);
      const Synset& predicted_synset =
        wordnet_->final(lang, word, path_assignments_[ii][jj]);
      // TODO(jbg): Stop using the proto buffer, as it's also stored as an index
      const string& predicted = corpus_->synset(predicted_synset.corpus_id());
      if (answer == predicted) {
        right += 1;
        outfile << "+ ";
      } else {
        wrong += 1;
        outfile << "- " << answer << " ";
      }
      outfile << predicted << endl;
    }
  }

  double val = double(right) / double(wrong + right);

  accuracy_file << val << endl;

  return val;
}


void LdawnState::WriteOther() {
  // Write the current state of the path assignments
  std::fstream outfile((output_ + ".path_assignments").c_str(), ios::out);
  for (int ii = 0; ii < (int)corpus_->num_docs(); ii++) {
    outfile << path_assignments_[ii].size() << " ";
    for (int jj = 0; jj < (int)path_assignments_[ii].size(); ++jj) {
      outfile << path_assignments_[ii][jj] << " ";
    }
    outfile << endl;
  }
  outfile.close();

  // Write a dot file representing the transition probabilities
  // between nodes.
  for (int tt = 0; tt < num_topics_; ++tt) {
    topic_walks_[tt].WriteDotTransitions(output_ + "." +
                                         lib_util::SimpleItoa(tt) + ".dot",
                                         wordnet_->debug_synsets(),
                                         corpus_.get());
  }
}


  /*
void toy_corpus(lda::TaggedCorpusPart& c) {
  vector<string> english_word_list;
  english_word_list.push_back("food");
  english_word_list.push_back("animal");
  english_word_list.push_back("dog");
  english_word_list.push_back("hound");
  english_word_list.push_back("pork");
  english_word_list.push_back("pig");
  english_word_list.push_back("hot dog");
  english_word_list.push_back("hotdog");
  english_word_list.push_back("gummy bear");

  vector<string> german_word_list;
  german_word_list.push_back("essen");
  german_word_list.push_back("tier");
  german_word_list.push_back("hund");
  german_word_list.push_back("schwein");
  german_word_list.push_back("schweinfleish");
  german_word_list.push_back("hotdog");
  german_word_list.push_back("gummibaer");

  vector<string> synset_list;
  synset_list.push_back("entity");
  synset_list.push_back("animal");
  synset_list.push_back("food");
  synset_list.push_back("gummy bear");
  synset_list.push_back("man's best friend");
  synset_list.push_back("charlotte's best friend");
  synset_list.push_back("man's best meal");
  synset_list.push_back("kobayashi's best friend");

  vector<string> tag_list;
  tag_list.push_back("n");
  tag_list.push_back("v");
  tag_list.push_back("det");
  tag_list.push_back("adj");
  tag_list.push_back("prep");

  vector<string> lang_list;
  lang_list.push_back("en");
  lang_list.push_back("de");

  lda::Vocab* english_words = c.add_words();
  english_words->set_lang(0);
  english_words->set_lang_string("en");
  int n = 0;
  BOOST_FOREACH(string ss, english_word_list) {
    lda::Vocab_Entry* e = english_words->add_terms();
    e->set_id(n++);
    e->set_name(ss);
  }

  lda::Vocab* german_words = c.add_words();
  german_words->set_lang(1);
  german_words->set_lang_string("de");
  n = 0;
  BOOST_FOREACH(string ss, german_word_list) {
    lda::Vocab_Entry* e = german_words->add_terms();
    e->set_id(n++);
    e->set_name(ss);
  }

  lda::Vocab* synsets = c.mutable_synsets();
  n = 0;
  BOOST_FOREACH(string ss, synset_list) {
    lda::Vocab_Entry* e = synsets->add_terms();
    e->set_id(n++);
    e->set_name(ss);
  }
}
  */


void toy_proto(WordNetFile* proto) {
  /*
   *              ------- (140) 1730 -------
   *           /                              \
   *        (60)              (10)            (80)
   *        1900: ------------2001:---------  2000:
   *          |               gummy_bear (5E)   |
   *          |               gummibaer (5D)    |
   *          |                                 |
   *          |   animal (5E)                   |   food (5E)
   *         /\   tier (5D)                    /\   essen (5D)
   *        /  \                             /    \
   *      (10) (30)                        (40)  (20)
   *      2010 2020                        3000  3010
   *  (5E) dog pig (10E)             (20E) pork  dog (5E)
   *(3E) hound schwein (20D)       (5D) schwein  hot dog (5E)
   *(2D)  hund              (15D) schweinfleish  hotdog (5E)
   *                                             hotdog (5D)
   *
   *
   *  Words (German):              Words (English):
   *  0     essen                  0     food
   *  1     tier                   1     animal
   *  2     hund                   2     dog
   *  3     schwein                3     hound
   *  4     schweinfleish          4     pork
   *  5     hotdog                 5     pig
   *  6     gummibaer              6     hot dog
   *  7     affen                  7     hotdog
   *  8     tollwut                8     gummy bear
   *
   */

  proto->set_root(1730);
  WordNetFile::Synset* s = proto->add_synsets();
  s->set_offset(1730);
  s->set_key("entity");
  s->add_children_offsets(1900);
  s->add_children_offsets(2000);
  s->set_hyponym_count(140);

  s = proto->add_synsets();
  s->set_offset(1900);
  s->set_key("animal");
  s->set_hyponym_count(60);
  s->add_children_offsets(2001);
  s->add_children_offsets(2010);
  s->add_children_offsets(2020);
  WordNetFile::Synset::Word* w = s->add_words();
  w->set_term_id(1);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("animal");
  w->set_count(5);
  w = s->add_words();
  w->set_term_id(1);
  w->set_lang_id(lib_corpora_proto::GERMAN);
  w->set_term_str("tier");
  w->set_count(5);

  s = proto->add_synsets();
  s->set_offset(2000);
  s->set_key("food");
  s->set_hyponym_count(80);
  s->add_children_offsets(2001);
  s->add_children_offsets(3000);
  s->add_children_offsets(3010);
  w = s->add_words();
  w->set_term_id(0);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("food");
  w->set_count(5);
  w = s->add_words();
  w->set_term_id(0);
  w->set_lang_id(lib_corpora_proto::GERMAN);
  w->set_term_str("essen");
  w->set_count(5);

  s = proto->add_synsets();
  s->set_offset(2001);
  s->set_key("gummy bear");
  s->set_hyponym_count(10);
  w = s->add_words();
  w->set_term_id(6);
  w->set_lang_id(lib_corpora_proto::GERMAN);
  w->set_term_str("gummibaer");
  w->set_count(5);
  w = s->add_words();
  w->set_term_id(8);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("gummy bear");
  w->set_count(5);

  s = proto->add_synsets();
  s->set_offset(2010);
  s->set_key("man's best friend");
  s->set_hyponym_count(10);
  w = s->add_words();
  w->set_term_id(2);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("dog");
  w->set_count(5);
  w = s->add_words();
  w->set_term_id(3);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("hound");
  w->set_count(3);
  w = s->add_words();
  w->set_term_id(2);
  w->set_lang_id(lib_corpora_proto::GERMAN);
  w->set_term_str("hund");
  w->set_count(2);

  s = proto->add_synsets();
  s->set_offset(2020);
  s->set_key("charlotte's best friend");
  s->set_hyponym_count(30);
  w = s->add_words();
  w->set_term_id(5);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("pig");
  w->set_count(10);
  w = s->add_words();
  w->set_term_id(3);
  w->set_lang_id(lib_corpora_proto::GERMAN);
  w->set_term_str("schwein");
  w->set_count(20);

  s = proto->add_synsets();
  s->set_offset(3000);
  s->set_key("man's best meal");
  s->set_hyponym_count(40);
  w = s->add_words();
  w->set_term_id(4);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("pork");
  w->set_count(50);
  w = s->add_words();
  w->set_term_id(3);
  w->set_lang_id(lib_corpora_proto::GERMAN);
  w->set_term_str("schwein");
  w->set_count(5);
  w = s->add_words();
  w->set_term_id(4);
  w->set_lang_id(lib_corpora_proto::GERMAN);
  w->set_term_str("schweinfleish");
  w->set_count(15);

  s = proto->add_synsets();
  s->set_offset(3010);
  s->set_key("kobayashi's best friend");
  s->set_hyponym_count(20);
  w = s->add_words();
  w->set_term_id(2);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("dog");
  w->set_count(5);
  w = s->add_words();
  w->set_term_id(6);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("hot dog");
  w->set_count(5);
  w = s->add_words();
  w->set_term_id(7);
  w->set_lang_id(lib_corpora_proto::ENGLISH);
  w->set_term_str("hotdog");
  w->set_count(5);
  w = s->add_words();
  w->set_term_id(5);
  w->set_lang_id(lib_corpora_proto::GERMAN);
  w->set_term_str("hotdog");
  w->set_count(5);
}
}  // end namespace topicmod_projects_ldawn
