from glob import glob
import codecs
from string import ascii_lowercase

from proto.corpus_pb2 import *
from corpus_reader import CorpusReader
from corpus_reader import DocumentReader
from nltk.tokenize.treebank import TreebankWordTokenizer

import nltk
import string

# TODO(jbg): Which tokenizer is being used should be a corpus-level option that is inherited
tokenizer = TreebankWordTokenizer()
word_tokenize = tokenizer.tokenize

punctuation_table = string.maketrans("","")

def reduce_speaker(speaker):
    speaker = speaker.split("(")[0]
    speaker = speaker.split(",")[0]
    speaker = speaker.replace(", Crossfire", "")
    speaker = speaker.replace("GOV. ", "")
    speaker = speaker.replace("SEN. ", "")
    speaker = speaker.replace("REP. ", "")
    
    speaker = speaker.strip()

    return speaker

class CrossfireCorpus(CorpusReader):
    def doc_factory(self, lang, filename):
        yield CrossfireDocument(filename, codecs.open(filename).read(), lang)

class CrossfireDocument(DocumentReader):
    def __init__(self, filename, raw, id, lang=ENGLISH):
        self._raw = raw
        self.lang = lang
        self._author_map = None

        self._id = None
        self._title = filename.rsplit("/", 1)[-1].split(".", 1)[0]

    def author_map(self):
        if self._author_map:
            return self._author_map
        
        authors = {}
        for ii in self._raw.split("\n"):
            if not ":" in ii:
                print("NO SPEAKER: %s" % ii)
                continue
            
            speaker, turn = ii.split(":", 1)
            speaker = reduce_speaker(speaker)

            # Speakers should only be uppercase
            if not any(x in ascii_lowercase for x in speaker):
                long_form = speaker
                for jj in authors:
                    if speaker in jj:
                        if len(jj) > len(long_form):
                            long_form = jj
                authors[speaker] = long_form
            else:
                print("INVALID SPEAKER %s %s" % (speaker, turn))

        self._author_map = authors
        return authors

    def author_turns(self):
        authors = self.author_map()

        for ii in self._raw.split("\n"):
            if not ":" in ii:
                print("NO SPEAKER: %s" % ii)
                continue
            
            speaker, turn = ii.split(":", 1)
            speaker = reduce_speaker(speaker)

            try:
                if speaker in authors:
                    yield authors[speaker], [x.translate(punctuation_table, \
                                                         string.punctuation) \
                                             for x in word_tokenize(turn.lower())]
                else:
                    print("BAD TURN: %s" % ii)
            except ValueError:
                print("BAD ENCODING!")

    def authors(self):
        return self.author_map().values()

    def tokens(self):
        for ss in self.sentences():
            for ww in ss:
                yield ww

    def sentences(self):
        for aa, ss in self.author_turns():
            yield ss

    def proto(self, num, language, authors, token_vocab, token_df, lemma_vocab,
              pos, synsets, stemmer):
        d = Document()
        assert language == self.lang

        # Use the given ID if we have it
        if self._id:
            d.id = self._id
        else:
            d.id = num

        d.title = self._title

        d.language = language
        tf_token = nltk.FreqDist()
        for ii in self.tokens():
            tf_token.inc(ii)

        for aa, ss in self.author_turns():
            s = d.sentences.add()
            s.author = authors[aa]
            for ii in ss:
                try:
                    token = token_vocab[ii]
                except KeyError:
                    print "Bad token in document %s" % d.title
                    continue
                    
                w = s.words.add()
                w.token = token_vocab[ii]
                w.lemma = lemma_vocab[stemmer(language, ii)]
                w.tfidf = token_df.compute_tfidf(ii, tf_token.freq(ii))
        return d
