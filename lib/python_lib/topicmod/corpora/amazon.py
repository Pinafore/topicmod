from glob import glob
import codecs

from proto.corpus_pb2 import *
from corpus_reader import CorpusReader
from corpus_reader import DocumentReader
import random

import nltk


class AmazonDocument(DocumentReader):

    def __init__(self, filename, lang):
        self.lang = lang
        self.internal_id = filename
        self._raw = u""
        self.author = u""
        self._rating = -1
        self._id = None

        for ii in codecs.open(filename, encoding='utf-8', errors='ignore'):
            # Extract the field
            start = ii.find(">")
            end = ii.rfind("<")
            if start < 0 or end < start:
                continue
            field_name = ii[1:start]
            content = ii[(start + 1):end]
            if field_name == u"review":
                self._raw += content
            elif field_name == u"author":
                self.author += content
            elif field_name == u"summary":
                self._raw += content
            elif field_name == u"rating":
                float_val = float(content.split()[0])
                self._rating = float_val

        # Normalize
        self._raw = self._raw.lower()

    def proto(self, num, language, authors, token_vocab, token_df, lemma_vocab,
              pos, synsets, stemmer):
        d = Document()
        assert language == self.lang

        # Use the given ID if we have it
        if self._id:
            d.id = self._id
        else:
            d.id = num

        d.language = language
        d.author = authors[self.author]
        d.rating = self._rating
        # Maybe split into sentences at some point?

        tf_token = nltk.FreqDist()
        for ii in self.tokens():
            tf_token.inc(ii)

        s = d.sentences.add()
        for ii in self.tokens():
            w = s.words.add()
            w.token = token_vocab[ii]
            w.lemma = lemma_vocab[stemmer(language, ii)]
            w.tfidf = token_df.compute_tfidf(ii, tf_token.freq(ii))
        return d

    def add_langauge(self, pattern, response_pattern, language=ENGLISH):
        self._response = response_pattern
        CorpusReader.add_langauge(self, pattern, language)

    def authors(self):
        yield self.author


class AmazonCorpus(CorpusReader):

    def doc_factory(self, lang, filename):
        try:
            yield AmazonDocument(filename, lang)
        except ValueError:
            print "Value error for doc", filename
            return
        except IOError:
            print "IOError for doc", filename
