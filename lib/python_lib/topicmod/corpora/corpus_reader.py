# Removing tagging for now
#from nlp.treetagger import tag_document
import os
from collections import defaultdict
from glob import glob
from math import log

import codecs
import random

import nltk
from nltk import FreqDist
from nltk.tokenize import wordpunct_tokenize
from nltk.tokenize import PunktWordTokenizer

from topicmod.ling.snowball_wrapper import Snowball
from topicmod.ling.stop import StopWords
from topicmod.util.sets import poll_iterator
from topicmod.ling.bigram_finder import BigramFinder, iterable_to_bigram, \
    iterable_to_bigram_offset

# Provides language IDs
from proto.corpus_pb2 import Corpus, Document
from proto.corpus_pb2 import ENGLISH, GERMAN, CHINESE, FRENCH, \
    SPANISH, ARABIC, DIXIE

# Lookup for the stopword corpus
LANGUAGE_ID = {ENGLISH: "english", GERMAN: "german", CHINESE: "chinese", \
                   FRENCH: "french", SPANISH: "spanish", ARABIC: "arabic", \
                   DIXIE: "english"}

sent_tokenizer = {}
for ii in LANGUAGE_ID:
    try:
        sent_tokenizer[ii] = nltk.data.load('tokenizers/punkt/%s.pickle' % \
                                                LANGUAGE_ID[ii])
    except LookupError:
        print("Error loading sentence tokenizer for %s" % LANGUAGE_ID[ii])
        None

word_tokenizer = PunktWordTokenizer()


def write_proto(filename, proto):
    f = open(filename, "wb")
    f.write(proto.SerializeToString())
    f.close()


class DocumentReader:
    """
    Base class that represents a document
    """

    def __init__(self, raw, lang=ENGLISH):
        self.lang = lang
        self._raw = raw

        self.author = ""

    def sentences(self):
        """
        Returns an iterator over sentences.
        """
        assert self.lang in sent_tokenizer, "%i lang missing" % self.lang

        for ii in sent_tokenizer[self.lang].tokenize(self._raw):
            yield word_tokenizer.tokenize(ii)

    def raw(self):
        """
        The raw text of the document
        """
        return self._raw

    def relations(self):
        return []

    def pos_tags(self):
        return []

    def authors(self):
        if self.author != "":
            yield self.author

    def prepare_document(self, num, language, authors):
        """
        The first step in creating a protocol buffer for a document;
        filling any document specific information.  Also creates
        a term frequency dictionary.
        """
        d = Document()

        if self.author:
            d.author = authors[self.author]
        d.language = language

        tf_token = nltk.FreqDist()
        for ii in self.sentences():
            for jj in ii:
                tf_token.inc(jj)

        return d, tf_token

    def proto(self, num, language, authors, token_vocab, df, lemma_vocab,
              pos, synsets, stemmer, bigram_vocab={}, bigram_list=None,
              bigram_normalize=None):

        d, tf = self.prepare_document(num, language, authors)

        for ii in self.sentences():
            s = d.sentences.add()

            s = self.fill_sentence(s, ii, language, token_vocab, lemma_vocab,
                                   pos, synsets, stemmer, bigram_vocab,
                                   tf, df, bigram_list, bigram_normalize)

        return d

    def fill_sentence(self, sentence, sentence_tokens, language,
                      token_vocab, lemma_vocab, pos, synsets, stemmer,
                      bigram_vocab, tf, df, bigram_list, bigram_normalize):
        """
        A method to fill a sentence protocol buffer from source tokens.
        """
        bigrams = defaultdict(str)
        if bigram_list:
            for pp, ww in iterable_to_bigram_offset(sentence_tokens,
                                                    bigram_list,
                                                    bigram_normalize):
                bigrams[pp] = ww

        index = 0
        for ii in sentence_tokens:
            w = sentence.words.add()
            w.token = token_vocab[ii]
            w.lemma = lemma_vocab[stemmer(language, ii)]
            w.tfidf = df.compute_tfidf(ii, tf.freq(ii))
            if index in bigrams:
                w.bigram = bigram_vocab[bigrams[index]]
            else:
                w.bigram = -1
            index += 1
        return sentence

    def synsets(self):
        return []

    def lemmas(self, stemmer):
        for jj in self.tokens():
            yield stemmer(self.lang, jj)

    def tokens(self):
        """
        A list of all the tokens in the document
        """

        for ii in self.sentences():
            for jj in ii:
                yield jj

#  def tagged_tokens(self):
#    """
#    A list of TreeTagTokens
#    """
#    return tag_document(self._raw, self.lang)


class DfCalculator:

    def __init__(self):
        self._df_counts = defaultdict(set)
        self._docs = set()

    def word_seen(self, doc, word):
        self._df_counts[word].add(doc)
        self._docs.add(doc)

    def compute_tfidf(self, word, tf):
        idf = log(len(self._docs)) - log(len(self._df_counts[word]))
        return tf * idf


class CorpusReader:
    """
    A collection of documents
    """

    def __init__(self, base, doc_limit=-1, bigram_limit=-1):
        self._file_base = base
        self._files = defaultdict(set)
        self._total_docs = 0

        self._bigram_finder = {}
        self._bigram_limit = bigram_limit

        self._author_freq = FreqDist()
        self._word_df = defaultdict(DfCalculator)
        self._word_freq = defaultdict(FreqDist)
        self._lemma_freq = defaultdict(FreqDist)
        self._bigram_freq = defaultdict(FreqDist)
        self._tag_freq = defaultdict(FreqDist)
        self._synset_freq = FreqDist()

        self._author_lookup = {}
        self._word_lookup = defaultdict(dict)
        self._lemma_lookup = defaultdict(dict)
        self._stop_words = defaultdict(set)
        self._pos_tag_lookup = defaultdict(dict)
        self._bigram_lookup = defaultdict(dict)
        self._synset_lookup = {}

        self._doc_limit = doc_limit
        self._stemmer = Snowball()

    def lang_iter(self, lang):
        print "DOC LIMIT %i" % self._doc_limit
        file_list = list(self._files[lang])

        doc_num = 0

        file_list.sort()
        random.seed(0)
        random.shuffle(file_list)

        if len(file_list) > 100:
            for ff in file_list:
                for dd in self.doc_factory(lang, ff):
                    if self._doc_limit > 0 and doc_num >= self._doc_limit:
                        return
                    doc_num += 1
                    yield dd
        else:
            file_list = list(self.doc_factory(lang, x) for x in file_list)
            for dd in poll_iterator(file_list):
                if self._doc_limit > 0 and doc_num >= self._doc_limit:
                    return
                doc_num += 1
                yield dd

    def __iter__(self):
        """
        Return documents.
        """

        # We have two different types of behavior depending on the number of
        # files.  If we have lots of files, then just go through them

        for ll in self._files:
            for ii in self.lang_iter(ll):
                yield ii

    def sample(self, num_docs=-1, rand_seed=0):
        """
        Iterate over a subset of the documents.  Given the same random
        seed, the results should be consistent.
        """
        raise NotImplementedError

    def build_vocab(self):
        """
        Create counts for all of the tokens.  Does care about lemmatization and
        will create separate vocab for that.  Also ignores tags.
        """
        self._author_freq = FreqDist()

        print "Building vocab:"
        doc = 0
        for ii in self:
            doc += 1
            if doc % 100 == 0:
                print("Doc %i / %i (total estimated)" % \
                          (doc, self._total_docs))
                self._total_docs = max(self._total_docs, doc)
            for jj in ii.authors():
                self._author_freq.inc(jj)
            for jj in ii.lemmas(self._stemmer):
                try:
                    jj.encode("utf-8", "replace")
                    self._lemma_freq[ii.lang].inc(jj)
                except ValueError:
                    None
            for jj in ii.tokens():
                try:
                    jj.encode("utf-8", "replace")
                    self._word_freq[ii.lang].inc(jj)
                    self._word_df[ii.lang].word_seen(doc, jj)
                except ValueError:
                    None
            for jj in ii.synsets():
                self._synset_freq.inc(jj)
            for jj in ii.pos_tags():
                self._tag_freq[ii.lang].inc(jj)
            for jj in ii.relations():
                self._tag_freq[ii.lang].inc(jj)

        self._total_docs = doc
        self.init_stop()

        if self._bigram_limit > 0:
            for ii in self._word_freq:
                bf = BigramFinder(language=LANGUAGE_ID[ii])
                self._bigram_finder[ii] = bf
                bf.set_counts(self._word_freq[ii])
                print("Finding bigrams in language %i" % ii)

            doc = 0
            for ii in self:
                lang = ii.language()
                doc += 1
                if doc % 100 == 0:
                    print("Doc %i / %i" % (doc, self._total_docs))
                self._bigram_finder[lang].add_ngram_counts(ii.tokens())

            print("Scoring bigrams")
            bigrams = {}

            for lang in self._word_freq:
                bf = self._bigram_finder[lang]
                bf.find_ngrams([])

                bigrams[lang] = bf.real_ngrams(self._bigram_limit)
                print("First 10 bigrams")
                for ii in bigrams[lang].keys()[:10]:
                    print("%s_%s" % ii)

            print("Creating new counts after subtracting bigrams")
            doc = 0
            for ii in self:
                doc += 1
                lang = ii.language()
                bf = self._bigram_finder[lang]
                if doc % 100 == 0:
                    print("Doc %i / %i" % (doc, self._total_docs))

#                for jj in ii.tokens():
#                    self._bigram_freq[lang].inc(jj)

                for jj in ii.sentences():
                    for kk in iterable_to_bigram(jj, bigrams[lang],
                                                 bf.normalize_word):
                        self._bigram_freq[lang].inc(kk)

    def init_stop(self):
        """
        Requires vocabulary to be built first to know which languages appear.
        """
        s = StopWords()
        self._stop_words = defaultdict(set)
        for ll in self._word_freq:
            language_name = LANGUAGE_ID[ll]
            print "Loading stopwords for", ll, " from", language_name
            try:
                self._stop_words[ll] = s[language_name]
            except IOError:
                print "Could not load stop words for", language_name
            print "Loaded", len(self._stop_words[ll]), "words."

            # Make sure lemmatized versions are also in
            temp_stop = list(self._stop_words[ll])
            for ii in temp_stop:
                self._stop_words[ll].add(self._stemmer(ll, ii))

    def doc_factory(self, lang, filename):
        raise NotImplementedError

    def fill_proto_vocab(self, frequency_count, vocab_generator, lookup, name):
        for ll in frequency_count:
            voc = vocab_generator()
            voc.language = ll
            word_id = 0
            for tt in frequency_count[ll]:
                word = voc.terms.add()
                word.id = word_id
                word.original = tt
                word.ascii = tt.encode("ascii", "replace")
                word.frequency = frequency_count[ll][tt]
                word.stop_word = tt in self._stop_words[ll]
                if tt in lookup[ll]:
                    assert lookup[ll][tt] == word_id
                else:
                    lookup[ll][tt] = word_id

                if word_id < 50 or (word_id < 1000 and "_" in word.ascii):
                    print("%s\t%i\t%s\t%s\t%i" % (name, word_id, word.ascii,
                                                  str(word.stop_word),
                                                  word.frequency))

                word_id += 1

    def fill_proto_language_independent_vocab(self, frequency_count,
                                              vocab_generator, lookup, name):
        word_id = 0
        for tt in frequency_count:
            if not tt:
                continue
            word = vocab_generator()
            word.id = word_id
            word.original = tt
            word.ascii = tt.encode("ascii", "replace")
            word.frequency = frequency_count[tt]

            lookup[tt] = word_id

            word_id += 1

    def new_section(self):
        """
        Create a new corpus section and return it
        """
        c = Corpus()

        self.fill_proto_vocab(self._word_freq, c.tokens.add,
                              self._word_lookup, "TOKEN")
        self.fill_proto_vocab(self._bigram_freq, c.bigrams.add,
                              self._bigram_lookup, "BIGRAM")
        self.fill_proto_vocab(self._lemma_freq, c.lemmas.add,
                              self._lemma_lookup, "LEMMA")
        self.fill_proto_vocab(self._tag_freq, c.pos.add,
                              self._pos_tag_lookup, "TAG")
        self.fill_proto_language_independent_vocab(self._author_freq,
                                                   c.authors.terms.add,
                                                   self._author_lookup,
                                                   "AUTHOR")
        self.fill_proto_language_independent_vocab(self._synset_freq,
                                                   c.synsets.terms.add,
                                                   self._synset_lookup,
                                                   "SYNSET")

        return c

    def add_language(self, pattern, language=ENGLISH):
        search = self._file_base + pattern
        print "SEARCH:", search
        for ii in glob(search):
            self._files[language].add(ii)
            self._total_docs += 1

    def write_proto(self, path, name, docs_in_sec=10000):
        self.build_vocab()
        section = self.new_section()
        doc_id = 0

        bigram_list = {}
        if self._bigram_limit > 0:
            for lang in self._bigram_finder:
                bf = self._bigram_finder[lang]
                bigram_list[lang] = bf.real_ngrams(self._bigram_limit)

        for lang in self._files:
            doc_num = 0
            section_num = 0

            filename = "%s/%s_%s_%i" % (path, \
                       name, LANGUAGE_ID[lang], section_num)
            print path

            for doc in self.lang_iter(lang):
                if doc_num >= docs_in_sec:
                    print "Done with section ", \
                        section_num, " we've written ", doc_id
                    # Write the file
                    write_proto(filename + ".index", section)

                    section = self.new_section()
                    section_num += 1
                    doc_num = 0
                    filename = "%s/%s_%s_%i" % (path, name, LANGUAGE_ID[lang],
                                                  section_num)
                if not os.path.exists(filename):
                    os.mkdir(filename)

                assert lang in self._word_lookup, "%i not in vocab, %s" % \
                    (lang, str(self._word_lookup.keys()))
                if doc_id % 100 == 0:
                    print "Writing out ", lang, filename, doc_id, "/", \
                        len(self._files[lang])

                if self._bigram_limit > 0:
                    bf = self._bigram_finder[lang]
                    doc_proto = doc.proto(doc_id, lang, self._author_lookup,
                                          self._word_lookup[lang],
                                          self._word_df[lang],
                                          self._lemma_lookup[lang],
                                          self._pos_tag_lookup[lang],
                                          self._synset_lookup, self._stemmer,
                                          self._bigram_lookup[lang],
                                          bigram_list[lang],
                                          bf.normalize_word)
                else:
                    doc_proto = doc.proto(doc_id, lang, self._author_lookup,
                                          self._word_lookup[lang],
                                          self._word_df[lang],
                                          self._lemma_lookup[lang],
                                          self._pos_tag_lookup[lang],
                                          self._synset_lookup, self._stemmer)

                write_proto("%s/%i" % (filename, doc_id), doc_proto)

                section.doc_filenames.append("%s_%s_%i/%i" % \
                                                 (name, LANGUAGE_ID[lang],
                                                  section_num, doc_id))
                doc_id += 1
                doc_num += 1

            # We don't want to mix languages, so we close out each section when
            # done with a language

            if doc_num > 0:
                write_proto(filename + ".index", section)
                section = self.new_section()
                doc_num = 0
                section_num += 1
                filename = "%s/%s_%s_%i" % (path, name, LANGUAGE_ID[lang],
                                            section_num)
                if not os.path.exists(filename):
                    os.mkdir(filename)
            print doc_id, " files written"
