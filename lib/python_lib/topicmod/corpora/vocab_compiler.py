#!/usr/bin/python
# -*- coding: utf-8 -*-

import codecs
from collections import defaultdict
import string

from nltk import FreqDist

from topicmod.util import flags
from topicmod.ling.snowball_wrapper import Snowball
from topicmod.corpora.proto.corpus_pb2 import Corpus

PUNCTUATION = string.punctuation + u'„“–´’'


class VocabCompiler:

    def __init__(self):
        self._vocab = defaultdict(FreqDist)
        self._authors = defaultdict(int)

    def addVocab(self, cp_path, exclude_stop, special_stop,
                 exclude_punctuation, exclude_digits, use_stem,
                 min_length):
        """
        Go through the corpus proto
        """
        cp = Corpus()
        cp.ParseFromString(open(cp_path, 'rb').read())

        single_letters = list(string.ascii_lowercase)

        source = cp.tokens
        if use_stem:
            source = cp.lemmas

        for ii in source:
            lang = ii.language
            for jj in ii.terms:
                word = jj.original

                if jj.stop_word and exclude_stop:
                    continue

                if exclude_punctuation and \
                        all(x in string.punctuation \
                                for x in word):
                    continue

                if word in special_stop:
                    continue

                if len(word) < min_length:
                    continue

                if word in single_letters:
                    continue

                if exclude_digits and all(x in string.digits for \
                                              x in word):
                    continue
                self._vocab[lang].inc(word, jj.frequency)

        print "Languages in this corpus: ", self._vocab.keys()

    def writeVocab(self, filename, vocab_limit, min_freq):
        o = codecs.open(filename, 'w', 'utf-8')
        num_terms = defaultdict(int)
        for ii in self._vocab:
            for jj in self._vocab[ii]:
                if num_terms[ii] <= vocab_limit and \
                        self._vocab[ii][jj] >= min_freq:
                    num_terms[ii] += 1
                    o.write(u"%i\t%s\n" % (ii, jj))
        o.close()
