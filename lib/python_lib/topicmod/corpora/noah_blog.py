
from glob import glob

from topicmod.corpora.corpus_reader import *


class NoahBlogPost(DocumentReader):

    def __init__(self, filename):
        self.lang = ENGLISH
        self.internal_id = filename
        self._raw = open(filename).read()

    def sentences(self):
        for ii in self._raw.split("\n"):
            yield ii.strip()


class NoahCorpus(CorpusReader):

    def add_language(self, pattern, language, doc_limit=-1, skip=0):
        files = list(glob(self._file_base + pattern))
        random.seed(0)
        random.shuffle(files)

        doc_index = 0
        for ii in files:
            if doc_index >= skip:
                self._files[language].add(ii)
            if doc_limit > 0 and len(self._files[language]) > doc_limit:
                break

    def doc_factory(self, lang, filename):
        try:
            yield NoahBlogPost(filename)
        except IOError:
            print "IOError for doc", filename
