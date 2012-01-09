from glob import glob
import codecs
import random

from proto.corpus_pb2 import *
from corpus_reader import CorpusReader
from corpus_reader import DocumentReader


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

    def prepare_document(self, num, language, authors):
        d, tf = DocumentReader.prepare_document(self, num, language, authors)

        assert language == self.lang

        # Use the given ID if we have it
        if self._id:
            d.id = self._id
        else:
            d.id = num
        d.rating = self._rating

        d.language = language
        d.author = authors[self.author]
        # Maybe split into sentences at some point?

        return d, tf

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
