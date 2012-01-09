# Adapted from nlp

from glob import glob
import os

from topicmod.corpora.corpus_reader import DocumentReader, CorpusReader
from topicmod.ling.tokens import tokens
from proto.corpus_pb2 import ENGLISH

from collections import defaultdict

DATE_TYPES = {"publication_month": "month", "publication_day_of_month": "day",
              "publication_year": "year", "pdm": "month", "pdy": "year",
              "pdd": "day"}

# Use the c version of ElementTree, which is faster, if possible:
#try:
#    from lxml.etree import cElementTree as ElementTree
#except ImportError:
#    from nltk.etree import ElementTree
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
        for ii in DATE_TYPES:
            for jj in raw.findall(ii):
                self.date_[DATE_TYPES[ii]] = int(jj.text)

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

        body = [x for x in raw.findall("body/body.content/block")
                if x.attrib["class"] == "full_text"]

        for ii in raw.findall("txt"):
            body.append(ii)

        self.paras_ = []
        for ii in body:
            if "<p>" in ii.text:
                for jj in ii.text.split("<p>"):
                    self.paras_.append(jj.replace("</p>", ""))
            else:
                for pp in ii.findall("p"):
                    self.paras_.append(pp.text)

    def num_paragraphs(self):
        return len(self.paras_)

    def paragraph_tokens(self, para, remove_people=False):
        for ii in tokens(self.paras_[para]):
            if remove_people and ii in self.people_tokens_:
                continue
            else:
                yield ii
        return

    def prepare_document(self, num, language, authors):
        d, tf = DocumentReader.prepare_document(self, num, language, authors)

        d.date.day = self.date_["day"]
        d.date.month = self.date_["month"]
        d.date.year = self.date_["year"]
        d.rating = self.rating

        assert language == self.lang
        d.id = num
        d.title = self.filename_ + "\t" + self.title_

        return d, tf

    def sentences(self):
        """
        This isn't really sentences, but as close as we can get without doing
        breaking of our own.
        """
        for ii in xrange(len(self.paras_)):
            sent = [x for x in self.paragraph_tokens(ii)]
            yield sent

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


class NewYorkTimesResponseReader(NewYorkTimesReader):

    def __init__(self, base, response_file, doc_limit=-1, bigram_limit=-1):
        NewYorkTimesReader.__init__(self, base, doc_limit, bigram_limit)
        self.read_response(response_file)

    def read_response(self, filename):
        self._responses = defaultdict(dict)
        for x in [x.split() for x in open(filename)]:
            year, month, day, val = x
            self._responses[(int(year), int(month))][int(day)] = float(val)

    def lookup(self, year, month, day):
        date = (year, month)
        candidates = [x for x in self._responses[date] if x <= day]

        if len(candidates) == 0:
            if month > 1:
                date = (year, month - 1)
            else:
                date = (year - 1, 12)

            if date in self._responses:
                candidates = self._responses[date]
            else:
                return 0.0
        if len(candidates) == 0:
            return -1
        else:
            candidate = max(candidates)
            return self._responses[date][candidate]

    def doc_factory(self, lang, filename):
        d = NewYorkTimesDocument(filename, lang)
        d.rating = self.lookup(d.date_["year"], d.date_["month"],
                               d.date_["day"])
        yield d
