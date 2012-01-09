# Adapted from nlp

import sys

from glob import glob
from random import shuffle, seed, random
from string import strip
from threading import Thread
from time import sleep, time

from topicmod.corpora.corpus_reader import *
from topicmod.ling.tokens import tokens
from topicmod.ling.bigram_finder import iterable_to_bigram_offset

from collections import defaultdict

DATE_TYPES = {"publication_month": "month", "publication_day_of_month": "day",
              "publication_year": "year"}

# Use the c version of ElementTree, which is faster, if possible:
try:
    from lxml.etree import cElementTree as ElementTree
except ImportError:
    from nltk.etree import ElementTree


class NewYorkTimesDocument(DocumentReader):

    def __init__(self, filename, lang):
        DocumentReader.__init__(self, "")
        self.filename = filename
        self.lang = lang
        self.parse_from_file(filename)

    def parse_from_string(self, string):
        raw = ElementTree.fromstring(string)
        self.parse_tree(raw)

    def language(self):
        return self.lang

    def parse_from_file(self, filename):
        self._raw = ElementTree.parse(filename)
        self.filename_ = filename
        self.parse_tree(self._raw)

    def parse_tree(self, raw):
        self.title_ = [x.text for x in raw.findall("head/title")]
        if len(self.title_) >= 1:
            self.title_ = self.title_[0]
        else:
            self.title_ = ""

        self.date_ = dict([(DATE_TYPES[x.attrib["name"]],
                            int(x.attrib["content"])) \
                               for x in raw.findall("head/meta") \
                               if x.attrib["name"] in DATE_TYPES])
        #print "DATE", self.date_
        self.byline_ = [x.text for x in raw.findall("body/body.head/byline")
                        if x.attrib["class"] == "normalized_byline"]

        # there are other entities (such as orgs and locations), but
        # we're not dealing with them for now
        try:
            self.people_ = set([pp.text.upper().split("(")[0].strip() for pp in
                                raw.findall("head/docdata/" +
                                            "identified-content/person")
                                if pp.text not in self.byline_])
        except AttributeError:
            self.people_ = []

        self.people_tokens_ = []
        for ii in self.people_:
            self.people_tokens_ += tokens(ii)

        self.people_tokens_ = set(self.people_tokens_)

        try:
            body = [x for x in raw.findall("body/body.content/block")
                    if x.attrib["class"] == "full_text"][0]

            self.paras_ = [pp.text for pp in body.findall("p")]
        except IndexError:
            # There are sometimes only headlines
            self.paras_ = []

        def num_paragraphs(self):
            return len(self.paras_)

    def paragraph_tokens(self, para, remove_people=False):
        for ii in tokens(self.paras_[para]):
            if remove_people and ii in self.people_tokens_:
                continue
            else:
                yield ii
        return

    def proto(self, num, language, authors, token_vocab, df, lemma_vocab,
              pos, synsets, stemmer, bigram_vocab={}, bigram_list=None,
              bigram_normalize=None):
        d = Document()
        assert language == self.lang
        d.id = num

        if self.author:
            d.author = authors[self.author]
        d.language = language
        d.title = self.filename_ + "\t" + self.title_

        d.date.day = self.date_["day"]
        d.date.month = self.date_["month"]
        d.date.year = self.date_["year"]

        tf_token = nltk.FreqDist()
        for ii in xrange(len(self.paras_)):
            for jj in self.paragraph_tokens(ii):
                tf_token.inc(jj)

        for ii in xrange(len(self.paras_)):
            s = d.sentences.add()

            bigrams = defaultdict(str)
            if bigram_list:
                tokens = self.paragraph_tokens(ii)
                for pp, ww in iterable_to_bigram_offset(tokens, bigram_list,
                                                        bigram_normalize):
                    bigrams[pp] = ww

            index = 0
            for jj in self.paragraph_tokens(ii):
                w = s.words.add()
                w.token = token_vocab[jj]
                w.lemma = lemma_vocab[stemmer(language, jj)]
                w.tfidf = df.compute_tfidf(jj, tf_token.freq(jj))
                if index in bigrams:
                    w.bigram = bigram_vocab[bigrams[index]]
                else:
                    w.bigram = -1
                index += 1
        return d

    def tokens(self, remove_people=False):
        for ii in xrange(len(self.paras_)):
            for jj in self.paragraph_tokens(ii, remove_people):
                yield jj
        return


class NewYorkTimesReader(CorpusReader):

    def add_language(self, pattern, language=ENGLISH):
        print "Looking for lang", language, "pattern", \
            self._file_base + pattern, "..."
        files = list(glob(self._file_base + pattern))

        for ii in files:
            self._files[language].add(ii)
            self._total_docs += 1
        print "done"

    def add_language_list(self, file, language=ENGLISH):
        for ii in [self._file_base + x.strip() for x in open(file)]:
            if os.path.exists(ii):
                self._files[language].add(ii)
                self._total_docs += 1

    def doc_factory(self, lang, filename):
        yield NewYorkTimesDocument(filename, lang)
