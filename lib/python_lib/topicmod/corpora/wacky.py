
import gzip
from topicmod.corpora.amazon import *
from proto.corpus_pb2 import *
from collections import defaultdict

from nltk import FreqDist


class WackyLine:

    def __init__(self, line):
        self.word, self.lemma, self.pos, self.offset, \
          self.parent, self.rel = line.lower().split("\t")
        self.offset = int(self.offset)
        self.parent = int(self.parent)


class WackyDocument(DocumentReader):

    def __init__(self, lang, sentences, title):
        self.lang = lang
        self._raw = None
        self.author = ""
        self.title = title
        self._sentences = sentences
        self._id = None

    def tokens(self):
        num_sentences = max(self._sentences) + 1
        for ii in xrange(num_sentences):
            for jj in self._sentences[ii]:
                yield jj.word

    def pos_tags(self):
        num_sentences = max(self._sentences) + 1
        for ii in xrange(num_sentences):
            for jj in self._sentences[ii]:
                yield jj.pos

    def lemmas(self, stemmer):
        num_sentences = max(self._sentences) + 1
        for ii in xrange(num_sentences):
            for jj in self._sentences[ii]:
                yield jj.lemma

    def chunked_nps(self, min_count=2):
        for ii in self._sentences:
            tokens = []
            count = 0
            in_np = False

            for jj in self._sentences[ii]:
                if jj.pos.startswith("np"):
                    if not in_np:
                        tokens.append("<")
                    in_np = True
                elif in_np:
                    tokens.append(">")
                    in_np = False
                    count += 1
                if not ">" in jj.word and not "<" in jj.word:
                    tokens.append(jj.word)

            if count >= min_count:
                yield " ".join(tokens)
            else:
                yield ""

    def relations(self):
        num_sentences = max(self._sentences) + 1
        for ii in xrange(num_sentences):
            for jj in self._sentences[ii]:
                yield jj.rel

    def proto(self, num, language, authors, token_vocab, token_df, lemma_vocab,
              pos_vocab, synset_vocab, stemmer):
        d = Document()
        assert language == self.lang

        if self._id:
            d.id = self._id
        else:
            d.id = num

        d.language = language
        d.title = self.title.strip()
        num_sentences = max(self._sentences) + 1

        tf_token = FreqDist()
        for ii in self.tokens():
            tf_token.inc(ii)

        for ii in xrange(num_sentences):
            s = d.sentences.add()
            for jj in self._sentences[ii]:
                w = s.words.add()
                w.token = token_vocab[jj.word]
                w.lemma = lemma_vocab[jj.lemma]
                w.pos = pos_vocab[jj.pos]
                w.relation = pos_vocab[jj.rel]
                w.parent = jj.parent
                w.offset = jj.offset
                w.tfidf = token_df.compute_tfidf(jj.word,
                                                 tf_token.freq(jj.word))
        return d


def latin_iter(input_file):
    line = input_file.readline()
    while line:
        yield line.decode("latin-1")
        line = input_file.readline()
    return


class WackyCorpus(CorpusReader):

    def add_language(self, pattern, language=ENGLISH):
        print "SEARCH", self._file_base + pattern
        for ii in glob(self._file_base + pattern):
            print ii
            self._files[language].add(ii)

    def doc_factory(self, lang, filename):
        content_file = gzip.open(filename, 'r')

        buffer = defaultdict(list)
        current_sentence = 0

        for ii in latin_iter(content_file):
            if ii.startswith("<s>"):
                continue
            elif ii.startswith(u'<text id'):
                title = ii.split("wikipedia:")[1].replace('">', '')
                print title
            elif ii.startswith("</s>"):
                current_sentence += 1
            elif ii.startswith("</text>"):
                if title and len(buffer) > 10:
                    yield WackyDocument(lang, buffer, title)
                buffer = defaultdict(list)
                title = ""
            else:
                buffer[current_sentence].append(WackyLine(ii))
