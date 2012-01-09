from nltk.stem.snowball import *
from topicmod.corpora.proto.corpus_pb2 import *

import string


class IdStemmer:

    def __init__(self):
        None

    def stem(self, word):
        return word


class MorphyStemmer:

    def __init__(self):
        from nltk.corpus import wordnet as wn
        self._wn = wn

    def stem(self, word):
        first_try = self._wn.morphy(word)
        if first_try:
            return unicode(first_try)
        else:
            return word


class Snowball:

    def __init__(self):
        self._stemmers = {}
        self._lang_lookup = {ENGLISH: 'english', GERMAN: 'german',
                             DIXIE: 'english'}
        self._trans = dict((ord(x), None) for x in \
                               string.punctuation.decode('utf-8'))

    def __call__(self, lang, word, remove_punc=True):
        """
        Stem
        """
        if not lang in self._stemmers:
            if lang in [CHINESE]:
                self._stemmers[lang] = IdStemmer()
            elif lang == ENGLISH or lang == DIXIE:
                self._stemmers[lang] = MorphyStemmer()
            else: # Assume it's handled by snowball
                assert lang in self._lang_lookup

                self._stemmers[lang] = SnowballStemmer(self._lang_lookup[lang])
        val = self._stemmers[lang].stem(word)

        return val
