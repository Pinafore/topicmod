
from csv import DictReader
from csv import QUOTE_MINIMAL
from glob import glob

# Provides language IDs
from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.corpora.corpus_reader import *

TRANSCRIPTION_INDEX = 7


class AlignedDocument(DocumentReader):

    def __init__(self, filename, lang):
        self._filename = filename
        self.lang = lang

        infile = codecs.open(filename, 'r', 'utf-8')

        # Ignore header, but check that is matches our assumptions
        assert infile.readline().split("\t")[TRANSCRIPTION_INDEX] == \
            "transcript;unicode"

        self._sentences = [x.split("\t")[7] for x in infile]

    def paired(self):
        """
        Returns filenames of documents paired to this one
        """
        d = {}
        if self.lang == ENGLISH:
            d[ARABIC] = self._filename.replace("translation", "source")
        elif self.lang == ARABIC:
            d[ENGLISH] = self._filename.replace("source", "translation")

        return d


class AlignedCorpus(CorpusReader):

    def paired_iterator(self, lang=ENGLISH):
        """
        Iterate through documents in one language, picking up their aligned
        comrades as you go.  Returns an iterator over dictionaries keyed by
        language.
        """
        for ii in self.lang_iter(lang):
            d = {}
            d[lang] = ii
            paired_documents = ii.paired()
            for jj in paired_documents:
                if jj in self._files and paired_documents[jj] in self._files[jj]:
                    docs_in_lang = list(self.doc_factory(jj, paired_documents[jj]))
                    assert len(docs_in_lang) == 1
                    d[jj] = docs_in_lang[0]
                else:
                    print jj, paired_documents[jj], "Not found"
            yield d
        return

    def add_language(self, pattern, language):
        files = list(glob(self._file_base + pattern))
        random.seed(0)
        random.shuffle(files)
        for ii in files:
            self._files[language].add(ii)

    def doc_factory(self, lang, filename):
        yield AlignedDocument(filename, lang)
