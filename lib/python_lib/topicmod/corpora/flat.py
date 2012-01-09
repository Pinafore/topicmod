from glob import glob
import codecs
import random
import string
import re

import nltk

from proto.corpus_pb2 import Document
from corpus_reader import CorpusReader
from corpus_reader import DocumentReader


DATE_REGEXP = re.compile("[dD]ate:.*")
MONTH_LOOKUP = {"jan": 1, "feb": 2, "mar": 3, "apr": 4, "may": 5,
                "jun": 6, "jul": 7, "aug": 8, "sep": 9, "oct": 10,
                "nov": 11, "dec": 12}


class FlatDocument(DocumentReader):

    def __init__(self, filename, lang):
        self.lang = lang
        self.author = None
        self.internal_id = filename
        self._raw = codecs.open(filename, encoding='utf-8',
                                errors='ignore').read()
        self._id = None
        self._filename = filename

        # Normalize
        self._raw = self._raw.lower()

    def proto(self, num, language, authors, token_vocab, token_df,
              lemma_vocab, pos, synsets, stemmer):
        d = Document()
        assert language == self.lang

        # Use the given ID if we have it
        if self._id:
            d.id = self._id
        else:
            d.id = num
        d.title = "/".join(self._filename.split("/")[-3:])
        # print d.title

        d.language = language

        tf_token = nltk.FreqDist()

        for ii in self.sentences():
            for jj in ii:
                tf_token.inc(jj)

        s = d.sentences.add()
        for ii in self.sentences():
            for jj in ii:
                w = s.words.add()
                w.token = token_vocab[jj]
                w.lemma = lemma_vocab[stemmer(language, jj)]
                w.tfidf = token_df.compute_tfidf(jj, tf_token.freq(jj))
        return d


class FlatHtmlDocument(FlatDocument):

    def __init__(self, filename, lang):
        FlatDocument.__init__(self, filename, lang)

        self._raw = nltk.clean_html(self._raw)

        # Remove e-mail quotes
        self._raw = self._raw.replace(">", "")


class FlatEmailDocument(FlatHtmlDocument):

    def __init__(self, filename, lang):
        FlatHtmlDocument.__init__(self, filename, lang)

        if "\n\n" in self._raw:
            header, self._raw = self._raw.split("\n\n", 1)
        else:
            header = self._raw
            self._raw = ""

        self.date_ = {"day": -1, "month": -1, "year": -1}

        try:
            date = DATE_REGEXP.findall(header)[0].split()
        except IndexError:
            print "Date not found"
            return

        try:
            if date[1][0] in string.letters:
                day, month, year = date[2:5]
            else:
                day, month, year = date[1:4]

            self.date_["day"] = int(day)
            self.date_["month"] = MONTH_LOOKUP[month.lower()[:3]]
            self.date_["year"] = int(year)

        except ValueError:
            print "Error parsing date", date

    def proto(self, num, language, authors, token_vocab, df, lemma_vocab,
              pos, synsets, stemmer):
        d = FlatHtmlDocument.proto(self, num, language, authors, token_vocab,
                                   df, lemma_vocab, pos, synsets, stemmer)

        d.date.day = self.date_["day"]
        d.date.month = self.date_["month"]
        d.date.year = self.date_["year"]

        return d


class FlatHtmlCorpus(CorpusReader):

    def doc_factory(self, lang, filename):
        try:
            yield FlatHtmlDocument(filename, lang)
        except IOError:
            print "IOError for doc", filename


class FlatCorpus(CorpusReader):

    def doc_factory(self, lang, filename):
        try:
            yield FlatDocument(filename, lang)
        except IOError:
            print "IOError for doc", filename


class FlatEmailCorpus(CorpusReader):

    def doc_factory(self, lang, filename):
        try:
            yield FlatEmailDocument(filename, lang)
        except IOError:
            print "IOError for doc", filename
