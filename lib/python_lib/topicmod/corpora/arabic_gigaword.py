
import gzip
from topicmod.corpora.corpus_reader import *


class ArabicGigaword(CorpusReader):

    def doc_factory(self, lang, filename):
        print "Reading", filename
        in_doc = False
        paras = []

        for ii in codecs.getreader("utf-8")(gzip.open(filename)):
            if ii.startswith(u"<TEXT>"):
                in_doc = True
            elif ii.startswith(u"</TEXT>"):
                yield DocumentReader("\n".join(paras), lang)
                paras = []
                in_doc = False
            else:
                if in_doc:
                    if ii.startswith(u"<P>"):
                        paras.append("\n")
                    elif not ii.startswith(u"</P>"):
                        paras.append(ii)
                else:
                    None

    def add_language(self, pattern, language=ARABIC):
        CorpusReader.add_language(self, pattern, language)
