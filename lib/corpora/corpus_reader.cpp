/*
 * Copyright 2009 Jordan Boyd-Graber
 *
 * Class to read protocol buffers and load them into corpus objects
 */

#include <map>
#include <string>
#include <vector>
#include <set>

#include "topicmod/lib/corpora/corpus_reader.h"

namespace lib_corpora {

void ReadVocab(const string& filename, vector< vector<string> >* vocab) {
  std::ifstream ifs(filename.c_str());
  string temp;
  int nwords = 0;

  cout << "Reading vocab from " << filename << " ... " << endl;
  while (getline(ifs, temp)) {
    ++nwords;
    int lang = lib_util::ParseLeadingIntValue(temp);
    if (lang >= (int) vocab->size()) {
      vocab->resize(lang + 1);
      cout << "  Language " << lang << endl;
    }
    int loc = temp.find('\t') + 1;
    assert(loc > 0 && loc < (int)temp.size());
    (*vocab)[lang].push_back(temp.substr(loc, temp.size()));
  }
  cout << "Done.  Read " << nwords << " words" << endl;
  assert(nwords > 0);
}

  /*
   * use_lemma - use lemma information, if available
   * remove_stop - remove stop words
   * min_words - min words in a document
   * train_limit - stop reading the protocol buffer once we've added that many documents (-1 for all)
   */
CorpusReader::CorpusReader(bool use_lemma, bool use_bigram, bool remove_stop,
                           int min_words, int train_limit, int test_limit) {
  use_lemma_ = use_lemma;
  use_bigram_ = use_bigram;
  remove_stop_ = remove_stop;
  min_words_ = min_words;
  train_limit_ = train_limit;
  test_limit_ = test_limit;

  // Bigrams are based off of raw tokens, so we can't use lemmas and bigrams at
  // the same time.
  assert(!(use_lemma_ && use_bigram_));
}

void CorpusReader::ReadProtobuf(const string& location, const string& filename,
                                bool is_test,
                                const vector< vector<string> >& vocab,
                                const vector< vector<string> >& pos,
                                Corpus* corpus) {
  // Read in the protocol buffer
  string combined = location + "/" + filename;
  fstream input((combined).c_str(), ios::in | ios::binary);
  cout << "Opening protobuffer: " << combined << " ... " << endl;
  assert(input);
  lib_corpora_proto::Corpus raw_corpus;
  assert(raw_corpus.ParseFromIstream(&input));

  // Create a vocab mapping
  vector<IntIntMap> vocab_mapping;
  CreateVocabMaps(raw_corpus, vocab, &vocab_mapping);

  // Create a synset mapping
  IntIntMap synset_mapping;
  CreateUnfilteredMap(raw_corpus.synsets(), &synset_lookup_, &synset_mapping);

  // Create an author mapping
  IntIntMap author_mapping;
  CreateUnfilteredMap(raw_corpus.authors(), &author_lookup_, &author_mapping);

  vector< set<int> > valid_pos;
  FindValidPos(raw_corpus, pos, &valid_pos);

  proto_doc doc;
  int docs_added = 0;
  int docs_ignored = 0;
  // For each document, open the file and read in the protocol buffer
  for (int dd = 0; dd < raw_corpus.doc_filenames_size(); ++dd) {
    const string doc_filename = raw_corpus.doc_filenames(dd);
    fstream input((location + "/" + doc_filename).c_str(),
                  ios::in | ios::binary);
    if (!doc.ParseFromIstream(&input)) {
      cout << "Failed to read doc " << location + "/" + doc_filename << endl;
      break;
    }
    int lang = doc.language();
    /*
      cout << "\nfile:" << location << ":" << doc_filename <<
      "(lang=" << lang << ")" << endl;
    */
    assert(lang < (int)vocab_mapping.size());
    BaseDoc* new_doc =
      DocumentFactory(doc, vocab_mapping[lang], valid_pos[lang], author_mapping,
                      synset_mapping, is_test);
    new_doc->set_title(doc.title());

    if (is_valid_doc(new_doc)) {
      AddDocument(corpus, new_doc);
      ++docs_added;
    } else {
      ++docs_ignored;
      delete new_doc;
    }
    if (is_test && test_limit_ > 0 && docs_added >= test_limit_) break;
    if (!is_test && train_limit_ > 0 && docs_added >= train_limit_) break;
  }

  cout << "     done adding " << docs_added << " documents, ignoring " <<
    docs_ignored << " (because they were too short)" << endl;

  // Update the corpus mapping
  corpus->UpdateMapping(author_lookup_, synset_lookup_);
}

void CorpusReader::AddDocument(Corpus* corpus, BaseDoc* new_doc) {
  corpus->AddDoc(new_doc);
}

bool CorpusReader::is_valid_doc(BaseDoc* doc) {
  if (doc->num_tokens() > min_words_) {
    return true;
  } else {
    return false;
  }
}

void CorpusReader::FindValidPos(const lib_corpora_proto::Corpus& corpus,
                                const vector< vector<string> >& filter_pos,
                                vector< set<int> >* valid_pos) {
  // Find the largest language and resize to that
  for (int ll = 0; ll < corpus.pos_size(); ++ll) {
    const Vocab& corpus_pos = corpus.pos(ll);
    cout << "POS lookup " << ll << "/" << corpus.pos_size();
    assert(corpus_pos.has_language());
    int lang = corpus_pos.language();
    cout << " loading for language " << lang << endl;
    if (lang <= (int)valid_pos->size()) valid_pos->resize(lang + 1);
  }

  // Create a pos mapping; this is stupid slow, but there are few
  // enough parts of speech that it shouldn't be a problem
  for (int ll = 0; ll < corpus.pos_size(); ++ll) {
    const Vocab& corpus_pos = corpus.pos(ll);
    assert(corpus_pos.has_language());
    int lang = corpus_pos.language();
    // If no pos filters have been specified, use them all
    if (lang >= (int)filter_pos.size()) continue;

    for (uint ii = 0; ii < filter_pos[ll].size(); ++ii) {
      for (int jj = 0; jj < corpus_pos.terms_size(); ++jj) {
        if (corpus_pos.terms(jj).original() == filter_pos[lang][ii]) {
          (*valid_pos)[lang].insert(corpus_pos.terms(jj).id());
          break;
        }
      }
    }
  }
}

void CorpusReader::CreateVocabMap(const Vocab& corpus_vocab,
                                  const vector< vector<string> >& filter_vocab,
                                  vector<IntIntMap>* lookup) {
  assert(corpus_vocab.has_language());
  int lang = corpus_vocab.language();
  if (lang >= (int)lookup->size()) lookup->resize(lang + 1);
  if (filter_vocab[lang].size() > 0) {
    cout << "Adding vocab for language " << lang << "(" <<
      corpus_vocab.terms().size() << ")" << endl;
    CreateFilteredMap(corpus_vocab, filter_vocab[lang], &(*lookup)[lang]);
  } else {
    cout << "Skipping language " << lang << endl;
  }
}

void CorpusReader::CreateVocabMaps(const lib_corpora_proto::Corpus& corpus,
                                   const vector< vector<string> >& filter_vocab,
                                   vector<IntIntMap>* lookup) {
  for (int ii = 0; ii < corpus.tokens_size(); ++ii) {
    if (use_lemma_) {
      CreateVocabMap(corpus.lemmas(ii), filter_vocab, lookup);
    } else if (use_bigram_) {
      CreateVocabMap(corpus.bigrams(ii), filter_vocab, lookup);
    } else {
      CreateVocabMap(corpus.tokens(ii), filter_vocab, lookup);
    }
  }
}

void CorpusReader::CreateUnfilteredMap(const Vocab& proto_voc,
                                       StringIntMap* lookup,
                                       IntIntMap* mapping) {
  for (int ii = 0; ii < proto_voc.terms_size(); ++ii) {
    const lib_corpora_proto::Vocab_Entry& word = proto_voc.terms(ii);
    string term = word.original();
    if (lookup->find(term) == lookup->end()) {
      int new_id = lookup->size();
      (*lookup)[term] = new_id;
      // cout << "Adding " << term << " with id " << new_id << endl;
    }
    (*mapping)[word.id()] = (*lookup)[term];
    // cout << "---------------" << endl;
  }
}

void CorpusReader::CreateFilteredMap(const Vocab& corpus_voc,
                                     const vector<string>& filter_voc,
                                     IntIntMap* id_lookup) {
  map<string, int> new_id;

  // RHS will be new vocab
  for (int ii = 0; ii < (int)filter_voc.size(); ++ii) {
    new_id[filter_voc[ii]] = ii;
  }

  // LHS will be old vocab
  for (int ii = 0; ii < corpus_voc.terms_size(); ++ii) {
    const lib_corpora_proto::Vocab_Entry& word = corpus_voc.terms(ii);
    string term = word.original();
    if (new_id.find(term) != new_id.end()) {
      (*id_lookup)[word.id()] = new_id[term];
      // cout << word.id() << "->" << new_id[term] << "(term)" << endl;
    }
  }
}

bool CorpusReader::AddWord(const Document_Sentence_Word& word,
                           int sentence, const IntIntMap& vocab,
                           const set<int>& pos, const IntIntMap& synsets,
                           BaseDoc* doc) {
  // If this corpus has pos information *and* we filter on pos
  // *and* this pos isn't there, then skip this word
  if (word.has_pos() && pos.size() > 0 && pos.find(word.pos()) == pos.end())
    return false;

  int token = word.token();
  if (use_lemma_ && word.has_lemma()) token = word.lemma();
  if (use_bigram_) {
    if (word.bigram() == -1)
      return false;
    else
      token = word.bigram();
  }
  if (vocab.find(token) == vocab.end()) return false;
  int normalized_token = vocab.find(token)->second;

  doc->AddWord(sentence, normalized_token);
  doc->Addtfidf(sentence, word.tfidf());
  return true;
}

void CorpusReader::FillDocument(const proto_doc& source, const IntIntMap& vocab,
                                const set<int>& pos, const IntIntMap& synsets,
                                BaseDoc* doc) {
  doc->time_ = Time(source.date().year(), source.date().month(),
                    source.date().day());

  int sentences_seen = 0;
  for (int ss = 0; ss < source.sentences_size(); ++ss) {
    const lib_corpora_proto::Document_Sentence sent = source.sentences(ss);
    int words_added = 0;
    for (int ww = 0; ww < sent.words_size(); ++ww) {
      const lib_corpora_proto::Document_Sentence_Word word = sent.words(ww);
      if (AddWord(word, sentences_seen, vocab, pos, synsets, doc))
        ++words_added;
    }
    if (words_added > 0) ++sentences_seen;
  }
}

ReviewReader::ReviewReader(bool use_lemma, bool use_bigram, bool remove_stop,
                           int min_words, int train_limit, int test_limit,
                           int rating_train_limit, int rating_test_limit,
                           const set<double>& filter_ratings)
  : CorpusReader(use_lemma, use_bigram, remove_stop, min_words, train_limit,
                 test_limit), filter_ratings_(filter_ratings),
    rating_train_limit_(rating_train_limit),
    rating_test_limit_(rating_test_limit) {}

bool ReviewReader::is_valid_doc(BaseDoc* doc) {
  double rating = static_cast<ReviewDoc*>(doc)->rating();

  bool under_rating_threshold;
  if (doc->is_test()) {
    under_rating_threshold = (rating_test_limit_ < 0 ||
                              rating_test_count_[rating] < rating_test_limit_);
  } else {
    under_rating_threshold = (rating_train_limit_ < 0 ||
                              rating_train_count_[rating] <
                              rating_train_limit_);
  }

  return CorpusReader::is_valid_doc(doc) &&
    (filter_ratings_.size() == 0 ||
     filter_ratings_.find(rating) != filter_ratings_.end()) &&
    under_rating_threshold;
}

void ReviewReader::AddDocument(Corpus* corpus, BaseDoc* doc) {
  double rating = static_cast<ReviewDoc*>(doc)->rating();
  if (doc->is_test())
    ++rating_test_count_[rating];
  else
    ++rating_train_count_[rating];

  assert(rating_test_limit_ < 0 ||
         rating_test_count_[rating] <= rating_test_limit_);
  assert(rating_train_limit_ < 0 ||
         rating_train_count_[rating] <= rating_train_limit_);
  CorpusReader::AddDocument(corpus, doc);
}

BaseDoc* ReviewReader::DocumentFactory(const proto_doc& source,
                                       const IntIntMap& vocab,
                                       const set<int>& pos,
                                       const IntIntMap& authors,
                                       const IntIntMap& synsets,
                                       bool is_test) {
  ReviewDoc* doc = new ReviewDoc(is_test, source.id(), source.language(),
                                 authors.find(source.author())->second,
                                 source.rating());

  BaseDoc* base_doc = doc;
  FillDocument(source, vocab, pos, synsets, base_doc);

  return doc;
}

SemanticCorpusReader::SemanticCorpusReader(bool use_lemma, bool use_bigram,
                                           bool remove_stop, int min_words,
                                           int train_limit, int test_limit)
  : CorpusReader(use_lemma, use_bigram, remove_stop, min_words, train_limit,
                 test_limit) {}

BaseDoc* SemanticCorpusReader::DocumentFactory(const proto_doc& source,
                                               const IntIntMap& vocab,
                                               const set<int>& pos,
                                               const IntIntMap& authors,
                                               const IntIntMap& synsets,
                                               bool is_test) {
  SemMlSeqDoc* doc = new SemMlSeqDoc(is_test, source.id(), source.language());

  BaseDoc* base_doc = doc;
  FillDocument(source, vocab, pos, synsets, base_doc);
  return doc;
}

bool SemanticCorpusReader::AddWord(const Document_Sentence_Word& word,
                                   int sentence, const IntIntMap& vocab,
                                   const set<int>& pos,
                                   const IntIntMap& synsets, BaseDoc* doc) {
  // If this corpus has pos information *and* we filter on pos
  // *and* this pos isn't there, then skip this word
  if (word.has_pos() && pos.size() > 0 && pos.find(word.pos()) == pos.end())
    return false;

  int token = word.token();
  int normalized_synset = -1;
  if (word.has_synset()) {
    int synset = word.synset();
    assert(synsets.find(synset) != synsets.end());
    normalized_synset = synsets.find(synset)->second;
  }
  if (use_lemma_ && word.has_lemma()) token = word.lemma();
  if (vocab.find(token) == vocab.end()) return false;
  int normalized_token = vocab.find(token)->second;

  static_cast<SemMlSeqDoc*>(doc)->
    AddWord(sentence, normalized_token, normalized_synset);
  static_cast<SemMlSeqDoc*>(doc)->Addtfidf(sentence, word.tfidf());
  return true;
}
}  // namespace lib_corpora
