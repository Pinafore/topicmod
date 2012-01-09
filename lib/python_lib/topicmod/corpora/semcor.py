#!/usr/bin/python

import sys
from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.corpora.corpus_reader import DocumentReader, CorpusReader
from topicmod.util.wordnet import load_wn
import re

import nltk

#from util.find_data import find_data
from glob import glob


def RawSemcor(filename):
    text = open(filename, 'r').read()
    tags = re.compile("<[^>]*>")
    return tags.sub("", text)


def SemcorFiles(root, version="2.1"):
    filename = "semcor%s/" % version
    print "Looking for semcor in ", filename
    root = find_data(filename)
    files = glob(root + "semcor-%s/*/tagfiles/*" % version)
    files.sort()
    return files


class Word:

    def __init__(self, line):
        line = line.strip()
        if line.endswith("</wf>"):
            line = line[:-5]
        else:
            raise "This isn't a word line: " + line

        if line.startswith("<wf "):
            line = line[4:]
        else:
            raise "This isn't a word line: " + line

        line = line.split('>')
        if len(line) != 2:
            raise "Cannot parse line: " + str(line)

        self.form = line[1]
        line = line[0].split(' ')

        self.rdf = None
        self.lemma = None
        self.wnsn = None

        for l in line:
            if l.startswith("cmd="):
                pass
            elif l.startswith("pos="):
                self.pos = l[4:]
            elif l.startswith("lemma="):
                self.lemma = l[6:]
            elif l.startswith("wnsn="):
                self.wnsn = int(l[5:].split(';')[0])
            elif l.startswith("lexsn="):
                self.key = l[6:]
                self.key = self.key.replace("(a)", "")
                self.key = self.key.replace("(p)", "")
            elif l.startswith("ot="):
                pass
            elif l.startswith("rdf="):
                self.rdf = l[4:]
            elif l.startswith("pn="):
                pass
            elif l.startswith("dc="):
                pass
            elif l.startswith("sep="):
                pass
            elif l.startswith("id="):
                pass
            else:
                raise "Unparsed word option: " + l

    def __str__(self):
        if self.lemma is None:
            return self.form
        elif self.rdf is not None:
            return self.form + "(" + self.rdf + "!)"
        elif self.wnsn is None:
            return self.form + "(" + self.pos + "['" + self.lemma + "'])"
        else:
            return self.form + "(" + self.pos + "['" + self.lemma + "'][" + \
                   str(self.wnsn - 1) + "])"

    def is_noun(self, include_all=True):
        return (self.pos == "NN" or self.pos == "NNS") and \
               (include_all or (self.lemma is not None and self.wnsn > 0))

    def is_verb(self, include_all=True):
        return (self.pos == "VB" or self.pos == "VBD" or self.pos == "VBZ" or
                self.pos == "VBN" or self.pos == "VBG" or self.pos == "VBP") \
                and (include_all or (self.lemma is not None and self.wnsn > 0))

    def is_sensible(self, include_all=True):
        return (include_all or (self.lemma is not None and self.wnsn > 0)) \
               and self.pos not in ("RB", "NNP", "NNPS")

    def synset_name(self):
        """
        This returns the corresponding synset object, which makes sure
        that its correctly represented.
        """
        key = "%s%%%s" % (self.lemma, self.key)

        if ";" in key:
            return None
        else:
            syn = wn.lemma_from_key(key).synset  # Was load_wn not called?
            return syn.name

    def lemmaized(self):
        return (self.lemma is not None)


class Punctuation:

    def __init__(self, line):
        line = line.strip()
        if line.startswith("<punc>"):
            line = line[6:]
        else:
            raise "This isn't a punctuation line: " + line

        if line.endswith("</punc>"):
            line = line[:-7]
        else:
            raise "This isn't a punctuation line: " + line

        self.punc = line

    def is_noun(self, include_all=True):
        return False

    def is_verb(self, include_all=True):
        return False

    def is_sensible(self, include_all=True):
        return False

    def lemmaized(self):
        return False

    def __str__(self):
        return self.punc


class Sentence:

    def __init__(self, sentlines):
        if not sentlines[0].startswith("<s snum=") or \
               not sentlines[0].strip().endswith(">"):
            raise "This isn't the beginning of a sentence: " + sentlines[0]
        self.snum = int(sentlines[0].strip()[8:-1])
        if sentlines[-1].strip() != "</s>":
            raise "This isn't the end of a sentence: " + sentlines[-1]

        self.words = []
        for line in sentlines[1:-1]:
            if line.startswith("<wf "):
                w = Word(line)
                if w.is_sensible(False):
                    self.words.append(w)
            elif line.startswith("<punc>"):
                #                self.words.append(Punctuation(line))
                pass
            else:
                raise "Unparsed line: " + line

    def nouns(self, include_all=True):
        return filter(lambda x: x.is_noun(include_all), self.words)

    def verbs(self, include_all=True):
        return filter(lambda x: x.is_verb(include_all), self.words)

    def nouns_verbs(self, include_all=True):
        return filter(lambda x: x.is_noun(include_all) or
                      x.is_verb(include_all), self.words)

    def sensible(self, include_all=True):
        return filter(lambda x: x.is_sensible(include_all), self.words)

    def __len__(self):
        return len(self.words)

    def __getitem__(self, k):
        return self.words[k]

    def __str__(self):
        return "{" + " ".join(map(lambda s: str(s), self.words)) + "}"

    def lemmaized(self):
        return filter(lambda x: x.lemmaized(), self.words)


class Paragraph:

    def __init__(self, parlines):
        if not  parlines[0].startswith("<p pnum=") or \
               not parlines[0].strip().endswith(">"):
            raise "This isn't the beginning of a paragraph: " + parlines[0]
        self.pnum = int(parlines[0].strip()[8:-1])
        if parlines[-1].strip() != "</p>":
            raise "This isn't the end of a paragraph: " + parlines[-1]

        sentlines = []
        self.sentences = []
        for line in parlines[1:-1]:
            if line.startswith("<s snum="):
                sentlines = []
            sentlines.append(line)
            if line.startswith("</s>"):
                self.sentences.append(Sentence(sentlines))

    def __str__(self):
        return "[" + "\n".join(map(lambda s: str(s), self.sentences)) + "]"


def parse(fname):
    f = file(fname)
    parlines = []
    paragraphs = []
    synthetic = False
    for line in f.readlines():
        if line.startswith("<p pnum="):
            parlines = []
            synthetic = False

        if line.startswith("<context "):
            continue

        # Create a synthetic paragraph
        if len(parlines) == 0 and line.startswith("<s snum="):
            parlines.append("<p pnum=-1>\n")
            synthetic = True

        if synthetic and line.startswith("</context>"):
            parlines.append("</p>\n")
            paragraphs.append(Paragraph(parlines))

        parlines.append(line)
        if line.startswith("</p>"):
            paragraphs.append(Paragraph(parlines))

    return paragraphs


class SemcorDocument(DocumentReader):

    def __init__(self, filename, lang=ENGLISH):
        self.lang = lang
        self.internal_id = filename
        self.author = u""

        self.sentences = []
        for pp in parse(filename):
            for ss in pp.sentences:
                self.sentences.append(ss)

        self.raw = " ".join([" ".join(x.form for x in ss.words)
                             for ss in self.sentences])

    def pos_tags(self):
        for ss in self.sentences:
            for ww in ss.words:
                yield ww.pos

    def synsets(self):
        for ss in self.sentences:
            for ww in ss.words:
                synset = ww.synset_name()
                if synset:
                    yield synset

    def lemmas(self, stemmer):
        for ss in self.sentences:
            for ww in ss.words:
                yield ww.lemma

    def tokens(self):
        for ss in self.sentences:
            for ww in ss.words:
                yield ww.form

    def proto(self, num, language, authors, token_vocab, token_df, \
              lemma_vocab, pos_tags, synsets, stemmer):
        assert self.lang == language
        d = Document()
        d.id = num
        d.language = language

        tf_token = nltk.FreqDist()
        for ii in self.tokens():
            tf_token.inc(ii)

        for ss in self.sentences:
            new_sent = d.sentences.add()
            for ww in ss.words:
                if ww.is_sensible():
                    new_word = new_sent.words.add()
                    new_word.token = token_vocab[ww.form]
                    new_word.pos = pos_tags[ww.pos]
                    synset = ww.synset_name()
                    if synset:
                        new_word.synset = synsets[synset]
                    new_word.lemma = lemma_vocab[ww.lemma]

                    new_word.tfidf = token_df.compute_tfidf(ii, \
                    tf_token.freq(ii))
        return d


class SemcorCorpus(CorpusReader):

    def add_language(self, pattern, language=ENGLISH):
        search = self._file_base + pattern
        print "Looking for pattern ", search
        for ii in glob(search):
            self._files[language].add(ii)

    def load_wn(self, location, version):
        globals()["wn"] = load_wn(version, location)

    def doc_factory(self, lang, filename):
        yield SemcorDocument(filename, lang)
