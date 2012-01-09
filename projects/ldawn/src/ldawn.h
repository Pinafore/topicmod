/*
 * ldawn.h
 *
 * Copyright 2007, Jordan Boyd-Graber
 *
 * LDA derivative that uses distributions over trees instead of regular topics
 */

#ifndef TOPICMOD_PROJECTS_LDAWN_SRC_LDAWN_H__
#define TOPICMOD_PROJECTS_LDAWN_SRC_LDAWN_H__

#include <boost/foreach.hpp>

#include <fstream>
#include <map>
#include <vector>
#include <string>

#include "topicmod/lib/util/strings.h"
#include "topicmod/lib/corpora/proto/corpus.pb.h"
#include "topicmod/lib/lda/sampler.h"
// Because of a bug in protoc, this must come *after* any includes of
// sparse hash, such as the one that lives in the lda corpus code.
#include "topicmod/projects/ldawn/src/wordnet.h"

// offset is how many positions in hyperparameter vector are taken up by
// top-level hyperparameters.
#define HYP_VECTOR_OFFSET 2

// step is how many terms in hyperparameter vector there are per node class
#define HYP_VECTOR_STEP 3



namespace topicmod_projects_ldawn {

using lib_corpora::MlSeqDoc;
using lib_corpora::SemMlSeqDoc;
using std::ios;
using std::fstream;
using lib_prob::IntMap;

class LdawnState : public lib_lda::MultilingState {
  private:
    std::vector<bool> doc_has_answer_;
    std::map<string, vector<double> > default_hyperparameters_;

  public:
    friend class Ldawn;
    LdawnState(int num_topics, string output, string wn_location);
    virtual void ChangePath(int doc, int index, int path, int topic);
    void ChangePathCount(int doc, int index, int topic, int path, int change);
    int path_assignment(int doc, int index) const;
    void ResetCounts();
    bool use_aux_topics();
    void SetWalkHyperparameters(const std::map<string, int>& lookup,
                                const vector< vector <double> >& vals);
    virtual double TopicLikelihood() const;
    virtual void WriteDocumentTotals();

    const WordNet* wordnet();
  protected:
    string wordnet_location_;
    std::vector<TopicWalk> topic_walks_;
    boost::scoped_ptr<WordNet> wordnet_;
    std::vector< std::vector <int> > path_assignments_;

    void InitializeOther();
    virtual void InitializeWsdAnswer();
    virtual void InitializeWordNet();
    virtual void InitializeAssignments(bool random_init);

    void ReadAssignments();
    void WriteOther();
    void PrintStatus(int iteration) const;

    virtual void WriteMultilingualTopic(int topic, int num_terms,
                                        std::fstream* outfile);
    virtual double WriteDisambiguation(int iteration) const;
};

class Ldawn : public lib_lda::MultilingSampler {
  public:
    friend void lhood();
    Ldawn(double alpha_sum, double alpha_zero, double lambda, int num_topics,
          string wn_location);
    bool use_aux_topics();

    vector<string> hyperparameter_names();
    vector<double> hyperparameters();
    void SetHyperparameters(const vector<double>& hyperparameters);
    double WriteLhoodUpdate(std::ostream* lhood_file, int iteration,
                            double elapsed);
  protected:
    void InitializeTemp();
    void InitializeState(const string& output_location, bool random_init);
    int SampleWordTopic(int doc, int index);
    double LogProbIncrement(int doc, int index);
    virtual void ResetTest(int doc);

    string wordnet_location_;
    vector<string> hyperparameter_names_;
    std::map<string, int> hyperparameter_lookup_;
    vector< vector<double> > hyperparameter_values_;
    LdawnState* state();
};

  // void toy_corpus(lda::TaggedCorpusPart& c);
void toy_proto(WordNetFile* proto);
}  // namespace project_ldawn

#endif  // TOPICMOD_PROJECTS_LDAWN_SRC_LDAWN_H_
