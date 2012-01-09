/*
 * Copyright 2009 Jordan Boyd-Graber
 * 
 * File to hold state of LDA-based sampler
 */
#include <algorithm>
#include <string>
#include <vector>

#include "topicmod/lib/lda/state.h"

DEFINE_bool(use_dense_doc, false, "Use a dense document distribution");

namespace lib_lda {

State::State(int num_topics,
             string output) {
  output_ = output;
  num_topics_ = num_topics;
}

double State::TopicDocProbability(int doc, int topic) {
  // If we use logprob increment for sampling hyperparameters, this needs to
  // change to LogProbability
  return docs_[doc]->LogNumerator(topic);
}

void State::SetAlpha(double val) {
  BOOST_FOREACH(shared_ptr<Multinomial>& d, docs_) d->SetPriorScale(val); // NOLINT
}

void State::SetLambda(double val) {
  BOOST_FOREACH(shared_ptr<Multinomial>& t, topics_) t->SetPriorScale(val); // NOLINT  
}

void State::ResetCounts() {
  BOOST_FOREACH(shared_ptr<Multinomial>& d, docs_) d->Reset(); // NOLINT
  BOOST_FOREACH(shared_ptr<Multinomial>& t, topics_) t->Reset(); // NOLINT
}

double State::TopicVocProbability(int topic, int term) {
  BOOST_ASSERT(topic >= 0);
  BOOST_ASSERT(topic < (int)topics_.size());
  return topics_[topic]->LogProbability(term);
}


int State::topic_assignment(int doc, int index) const {
    return topic_assignments_[doc][index];
}

void State::InitializeAssignments(bool random_init) {
  int num_docs = corpus_->num_docs();
  topic_assignments_.resize(num_docs);

  for (int ii = 0; ii < num_docs; ii++) {
    topic_assignments_[ii].resize(corpus_->seq_doc(ii)->num_tokens());
    for (int jj = 0; jj < corpus_->seq_doc(ii)->num_tokens(); ++jj) {
      topic_assignments_[ii][jj] = -1;
      if (random_init && !corpus_->seq_doc(ii)->is_test())
        ChangeTopic(ii, jj, rand() % num_topics_);
    }
  }
  cout << "Initialized assignments" << endl;
}

void State::InitializeDocs(double alpha_sum, double alpha_zero_scale) {
  int num_documents = corpus_->num_docs();

  // doc_counts is M x K
  // doc_sum is a vector of size M
  docs_.resize(num_documents);

  boost::shared_ptr<gsl_vector> prior;
  if (alpha_zero_scale != 1.0) {
    prior.reset(gsl_vector_alloc(num_topics_), gsl_vector_free);
    gsl_vector_set_all(prior.get(), 1.0);
    gsl_vector_set(prior.get(), 0, alpha_zero_scale);
  }

  for (int ii = 0; ii < num_documents; ii++) {
    if (alpha_zero_scale != 1.0 || FLAGS_use_dense_doc) {
      DenseMultinomial* mult = new DenseMultinomial();
      mult->Initialize(prior, alpha_sum);
      docs_[ii].reset(mult);
    } else {
      docs_[ii].reset(new SparseMultinomial());
      docs_[ii]->Initialize(num_topics_, alpha_sum);
    }
  }
  cout << "Initialized counts for " << num_documents << " documents with " <<
    num_topics_ << " topics." << endl;
}

void State::InitializeTopics(double lambda) {
  cout << "Initializing base topics." << endl;
  topics_.resize(num_topics_);
  for (int ii = 0; ii < num_topics_; ii++) {
    topics_[ii].reset(new SparseMultinomial());
    topics_[ii]->Initialize(corpus_->vocab_size(DEFAULT_LANG), lambda);
  }
}

void State::InitializeOther() {}

void State::Initialize(lib_corpora::MlSeqCorpus* corpus,
                       bool random_init,
                       double alpha_sum,
                       double alpha_zero,
                       double lambda) {
  cout << "Running base state initialization." << endl;
  corpus_.reset(corpus);
  InitializeDocs(alpha_sum, alpha_zero);
  InitializeTopics(lambda);
  InitializeOther();

  // This comes last, as it might change the others if random_init is true
  InitializeAssignments(random_init);
}

const string& State::output_path() {
  return output_;
}

void State::ChangeTopic(int doc, int index, int topic) {
  /*
  cout << "M: " << (int)topic_assignments_.size() <<
    " N: " <<  (int)topic_assignments_[doc].size() << endl;
  */

  BOOST_ASSERT(doc < (int)topic_assignments_.size());
  BOOST_ASSERT(index < (int)topic_assignments_[doc].size());
  BOOST_ASSERT(doc >= 0);
  BOOST_ASSERT(index >= 0);

  int current_topic = topic_assignments_[doc][index];
  // We only decrement the current topic assignment if it's valid.
  if (current_topic >= 0) {
    ChangeTopicCount(doc, index, current_topic, -1);
  }

  // Likewise, we only increment the new topic if it's valid.
  topic_assignments_[doc][index] = topic;
  if (topic >= 0) {
    ChangeTopicCount(doc, index, topic, +1);
  }

  BOOST_ASSERT(topic_assignments_[doc][index] == topic);
}

void State::ChangeTopicCount(int doc, int index, int topic, int change) {
  // We always update the doc statistics for all words.
  int term = (*corpus_->seq_doc(doc))[index];
  docs_[doc]->ChangeCount(topic, change);
  topics_[topic]->ChangeCount(term, change);
}

void State::WriteTopicsPreamble(fstream* outfile) { // NOLINT
  (*outfile) << "Num docs:   " << corpus_->num_docs() << endl;
  (*outfile) << "Num tokens: " << corpus_->num_tokens() << endl;
  (*outfile) << "Num types:  " << corpus_->vocab_size(DEFAULT_LANG) << endl;\
  (*outfile) << endl;
}

  /*
   * TODO(jbg) : Test to see if this actually works; this has only
   * been used in derived classes.
   */
void State::WriteTopics(int num_terms) {
  fstream outfile((output_ + ".topics").c_str(), ios::out);
  WriteTopicsPreamble(&outfile);
  lib_corpora::MlVocab* vocab = static_cast<lib_corpora::MlSeqCorpus*>
    (corpus_.get())->vocab();
  for (int ii = 0; ii < num_topics_; ++ii) {
    WriteTopic(ii, num_terms, (*vocab)[DEFAULT_LANG], &outfile, true);
  }
}

void MultilingState::SetAlpha(double val) {
  BOOST_FOREACH(std::vector<VocDist>& lang, multilingual_topics_) { // NOLINT
    BOOST_FOREACH(VocDist& t, lang) t.SetPriorScale(val); // NOLINT
  }
  State::SetAlpha(val);
}

void MultilingState::ResetCounts() {
  BOOST_FOREACH(std::vector<VocDist>& lang, multilingual_topics_) { // NOLINT
    BOOST_FOREACH(VocDist& t, lang) t.Reset(); // NOLINT
  }
  State::ResetCounts();
}

void MultilingState::WriteTopics(int num_terms) {
  fstream outfile((output_ + ".topics").c_str(), ios::out);
  WriteTopicsPreamble(&outfile);
  for (int ii = 0; ii < num_topics_; ++ii) {
    WriteMultilingualTopic(ii, num_terms, &outfile);
  }
}

void MultilingState::WriteTopicsPreamble(fstream* outfile) {
  (*outfile) << "Num docs:   " << corpus_->num_docs() << endl;
  for (int ll = 0; ll < num_langs_; ++ll) {
    (*outfile) << "Lang " << ll << endl;
    (*outfile) << "  Num tokens: " << corpus_->num_ml_tokens(ll) << endl;
    (*outfile) << "  Num types:  " << corpus_->vocab_size(ll) << endl;
  }
  (*outfile) << endl;
}

void State::WriteTopicHeader(int topic, fstream* outfile) {
  (*outfile) << "-----------------------------------------" << endl;
  (*outfile) << "          Topic " << topic << endl;
  (*outfile) << "-----------------------------------------" << endl;
}

void State::WriteTopic(int topic, int num_terms,
                       const vector<string>& vocab,
                       fstream* outfile, bool header) {
  if (header) WriteTopicHeader(topic, outfile);
  topics_[topic]->WriteTopWords(num_terms, vocab, outfile);
}

void MultilingState::WriteMultilingualTopic(int topic, int num_terms,
                                            fstream* outfile) {
  WriteTopicHeader(topic, outfile);
  for (int ll = 0; ll < num_langs_; ++ll) {
    (*outfile) << "          *** Lang " << ll << " (" <<
      multilingual_topics_[ll][topic].sum() << ") ***" << endl;
    lib_corpora::MlVocab* vocab = static_cast<lib_corpora::MlSeqCorpus*>
      (corpus_.get())->vocab();
    multilingual_topics_[ll][topic].WriteTopWords(num_terms,
                                                  (*vocab)[ll], outfile);
  }
}

void State::WriteDocumentTotals() {
  cout << "Writing document summaries to " << output_ + ".doc_totals" << endl;
  fstream outfile((output_ + ".doc_totals").c_str(), ios::out);
  BOOST_FOREACH(const shared_ptr<Multinomial>& dd, docs_) {
    for (int kk = 0; kk < num_topics_; ++kk) {
      outfile << dd->Probability(kk) << " ";
    }
    outfile << endl;
  }
  outfile.close();

  outfile.open((output_ + ".doc_id").c_str(), ios::out);
  int D = corpus_->num_docs();
  for (int ii = 0; ii < D; ++ii) {
    int lang = corpus_->seq_doc(ii)->language();
    outfile << lang << " " << corpus_->doc(ii)->id() <<
      "\t" << corpus_->doc(ii)->title() << endl;
  }
}

void State::ReadOther() {}

  // TODO(jbg): Make this insensitive to changing the save_delay flag.  As it
  // stands, changing the save_delay variable can make results no longer
  // reproducible.
int State::ReadState(int save_delay) {
  ReadAssignments();
  ReadOther();

  int iteration = ReadLikelihood();
  if (iteration % save_delay != 0) {
    // If we went past the last save point, then back time up to that point.
    iteration = (iteration / save_delay) * save_delay;
  }
  return iteration;
}

void State::PrintStatus(int iteration) const {
  int total_assignments = 0;
  int num_topics_used = 0;
  for (int ii = 0; ii < num_topics_; ++ii) {
    int usage = topics_[ii]->sum();
    if (usage > 0) num_topics_used += 1;
    total_assignments += usage;
  }
  cout << "There are " << num_topics_used << " topics using " <<
    total_assignments << " words." << endl;
}

int State::ReadLikelihood() {
  string line;
  int iteration = 0;

  ifstream myfile((output_ + ".lhood").c_str());
  if (myfile.is_open()) {
    while (!myfile.eof()) {
      getline(myfile, line);
      iteration = std::max(lib_util::ParseLeadingIntValue(line), iteration);
    }
    myfile.close();
  }
  return iteration;
}

vector<double> State::ReadHyperparameters(int dim) {
  vector<double> p(dim);
  fstream infile((output_ + ".params").c_str(), ios::in);
  double param;
  for (int ii = 0; ii < dim; ++ii) {
    assert(infile >> param);
    p[ii] = param;
  }
  return p;
}

void State::WriteHyperparameters(vector<double> p) {
  fstream outfile((output_ + ".params").c_str(), ios::out);
  for (int ii = 0; ii < (int)p.size(); ii++) {
    outfile << std::setprecision(15) << p[ii] << " ";
  }
  outfile << endl;
}

void State::ReadAssignments() {
  cout << "Reading assignments from " << output_ + ".topic_assignments" <<
    endl;
  fstream infile((output_ + ".topic_assignments").c_str(), ios::in);
  int topic;
  int num_words;
  for (int ii = 0; ii < (int)topic_assignments_.size(); ii++) {
    assert(infile >> num_words);
    assert(num_words == topic_assignments_[ii].size());
    for (int jj = 0; jj < num_words; ++jj) {
      assert(infile >> topic);
      ChangeTopic(ii, jj, topic);
    }
  }
}

void State::WriteState(bool verbose_assignments, bool mallet_assignments) {
  WriteAssignments();
  if (verbose_assignments)
    WriteVerboseAssignments();
  if (mallet_assignments)
    WriteMalletAssignments();
  WriteOther();
}

void State::WriteAssignments() {
  fstream outfile((output_ + ".topic_assignments").c_str(), ios::out);
  for (int ii = 0; ii < (int)topic_assignments_.size(); ii++) {
    outfile << topic_assignments_[ii].size() << " ";
    for (unsigned int jj = 0; jj < topic_assignments_[ii].size(); ++jj) {
      outfile << topic_assignments_[ii][jj] << " ";
    }
    outfile << endl;
  }
}

void State::WriteMalletAssignments() {
  fstream outfile((output_ + ".mallet_assignments").c_str(), ios::out);

  lib_corpora::MlSeqCorpus* corpus =
    static_cast<lib_corpora::MlSeqCorpus*>(corpus_.get());
  lib_corpora::MlVocab* vocab = corpus->vocab();

  for (int ii = 0; ii < (int)topic_assignments_.size(); ii++) {
    lib_corpora::MlSeqDoc* d = corpus->seq_doc(ii);
    int lang = d->language();
    for (unsigned int jj = 0; jj < topic_assignments_[ii].size(); ++jj) {
      int term = (*d)[jj];
      outfile << d->id() << " NA ";
      outfile << jj << " " << term << " ";
      outfile << (*vocab)[lang][term] << " ";
      outfile << topic_assignments_[ii][jj] << endl;
    }
  }
}

void State::WriteVerboseAssignments() {
  string outputDir = output_ + "_topic_assign";
  Corpus protocorpus;

  int num_docs = corpus_->num_docs();
  for (int dd = 0; dd < num_docs; dd++) {
    lib_corpora::MlSeqDoc* curdoc = corpus_->seq_doc(dd);

    Document protodoc;
    protodoc.set_id(curdoc->id());
    protodoc.set_title(curdoc->title());

    Document_Sentence* sent = protodoc.add_sentences();
    int num_words = curdoc->num_tokens();
    for (int ww = 0; ww < num_words; ww++) {
      Document_Sentence_Word* word = sent->add_words();
      word->set_token((*curdoc)[ww]);
      word->set_lemma((*curdoc)[ww]);
      word->set_topic(topic_assignment(dd, ww));
      word->set_tfidf(curdoc->gettfidf(ww));
    }

    if (curdoc->time_.day() != -1 || curdoc->time_.month() != -1
                                  || curdoc->time_.year() != -1) {
      Document_Time* time = protodoc.mutable_date();
      time->set_day(curdoc->time_.day());
      time->set_month(curdoc->time_.month());
      time->set_year(curdoc->time_.year());
    }

    // string filename = "assign/" + curdoc->title();
    std::ostringstream stm;
    stm << curdoc->id();
    string filename = "assign/" + stm.str();
    string outfilename = outputDir + "/" + filename;
    fstream outfile(outfilename.c_str(), ios::out);
    protodoc.SerializeToOstream(&outfile);
    outfile.close();

    protocorpus.add_doc_filenames(filename);
  }

  lib_corpora::MlVocab* curvoc = corpus_->vocab();
  int num_langs = curvoc->size();
  for (int ll = 0; ll < num_langs; ll++) {
    lib_corpora_proto::Vocab* vocab = protocorpus.add_tokens();
    lib_corpora_proto::Vocab* lemmas = protocorpus.add_lemmas();
    // TODO(ynhu): Check to make sure this language mapping is correct
    vocab->set_language(lib_corpora_proto::Language(ll));
    for (int ww = 0; ww < (*curvoc)[ll].size(); ww++) {
      string word = (*curvoc)[ll][ww];
      lib_corpora_proto::Vocab_Entry* term = vocab->add_terms();
      term->set_id(ww);
      term->set_original(word);
      term = lemmas->add_terms();
      term->set_id(ww);
      term->set_original(word);
    }
  }
  protocorpus.set_num_topics(num_topics_);
  string indexFile = "doc_voc.index";
  string indexoutfilename = outputDir + "/" + indexFile;
  fstream outfile(indexoutfilename.c_str(), ios::out);
  protocorpus.SerializeToOstream(&outfile);
  outfile.close();
}

void State::WriteOther() {
}

double State::DocLikelihood() const {
  double val = 0.0;
  BOOST_FOREACH(const shared_ptr<Multinomial>& dd, docs_) {
    val += dd->LogLikelihood();
  }
  return val;
}

double State::TopicLikelihood() const {
  double val = 0.0;
  BOOST_FOREACH(const shared_ptr<Multinomial>& tt, topics_) {
    val += tt->LogLikelihood();
  }
  return val;
}

State::~State() {}

MultilingState::MultilingState(int num_topics, string output)
  : State(num_topics, output) {
  num_langs_ = 0;
}

void MultilingState::Initialize(lib_corpora::MlSeqCorpus* corpus,
                                bool random_init,
                                double alpha_sum,
                                double alpha_zero,
                                double lambda) {
  cout << "Running MultilingState initialization. " << endl;
  num_langs_ = corpus->num_languages();
  State::Initialize(corpus, random_init, alpha_sum, alpha_zero, lambda);
}


double MultilingState::TopicLikelihood() const {
  double val = 0.0;
  BOOST_FOREACH(const std::vector<VocDist>& lang,
                multilingual_topics_) {
    BOOST_FOREACH(const VocDist& tt, lang) val += tt.LogLikelihood();
  }
  return val;
}


void MultilingState::PrintStatus(int iteration) const {
  for (int ii = 0; ii < num_langs_; ++ii) {
    int sum_assigned = 0;
    int topics_used = 0;
    for (int jj = 0; jj < num_topics_; ++jj) {
      if (multilingual_topics_[ii][jj].sum() > 0) topics_used+=1;
      sum_assigned += multilingual_topics_[ii][jj].sum();
    }
    cout << "Language " << ii << " used " << sum_assigned << " words in "
         << topics_used << " topics." << endl;
  }
}

void MultilingState::ChangeTopicCount(int doc, int index,
                                      int topic, int change) {
  // We always update the doc statistics for all words.
  int term = (*corpus_->seq_doc(doc))[index];
  int lang = corpus_->seq_doc(doc)->language();
  docs_[doc]->ChangeCount(topic, change);
  multilingual_topics_[lang][topic].ChangeCount(term, change);
}


double MultilingState::TopicVocProbability(int lang,
                                           int topic, int term) {
  BOOST_ASSERT(lang >= 0);
  BOOST_ASSERT(topic >= 0);
  BOOST_ASSERT(lang < (int)multilingual_topics_.size());
  BOOST_ASSERT(topic < (int)multilingual_topics_[lang].size());
  // cout << "Count : " << multilingual_topics_[lang][topic].count(term)
  // << " size: " << multilingual_topics_[lang][topic].size() << " sum: "
  // << multilingual_topics_[lang][topic].sum() << endl;
  return multilingual_topics_[lang][topic].LogProbability(term);
}

void MultilingState::InitializeTopics(double lambda) {
  cout << "Initializing multilingual (" << num_langs_ << ") topics." << endl;
  multilingual_topics_.resize(num_langs_);
  for (int ll = 0; ll <  num_langs_; ll++) {
    int vocab_size = corpus_->vocab_size(ll);

    if (vocab_size == 0) {
      // If there are no words in that language, then we don't need to
      // create a topic for it.
      continue;
    }

    cout << "Resizing " << ll << " to be size " << vocab_size << endl;
    multilingual_topics_[ll].resize(num_topics_);
    for (int ii = 0; ii < num_topics_; ii++) {
      multilingual_topics_[ll][ii].Initialize(vocab_size,
                                              lambda);
    }
  }
}
}
