
# Wrapper for the nltk stopwords function; allows us to take precidence if we
# don't like their stopwords (or if we have a language they don't)

from collections import defaultdict
from nltk.corpus import stopwords
import os
import codecs

class StopWords:
    def __init__(self, path="../../lib/python_lib/topicmod/data/stop/"):
        self._stop = defaultdict(set)
        self._path = path

    def load(self, lang):
        filename = "%s/%s.dat" % (self._path, lang)
        try:
            self._stop[lang] = set(unicode(x.strip()) for x in
                                   codecs.open(filename, encoding='utf-8')
                                   if x.strip() != "")
        except IOError:
            print "Falling back to NLTK stopwords", filename, "didn't work"
            self._stop[lang] = set(unicode(x) for x in stopwords.words(lang))
        return self._stop[lang]

    def __getitem__(self, lang):
        if not lang in self._stop:
            self.load(lang)
        return self._stop[lang]
