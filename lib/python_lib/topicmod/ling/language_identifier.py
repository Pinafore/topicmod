
from collections import defaultdict

import codecs
from glob import glob
from PyML import *

import nltk
from nltk import FreqDist
from nltk.util import ingrams

LANG_LOOKUP = {'en': 'english', 'de': 'german'}

class TrainingData:
    def __init__(self):
        self.label_names = []
        self.labels = []
        self.features = []
        self.id = []

    def AddInstance(self, lang, line, ngram_length):
        if not lang in self.label_names:
            self.label_names.append(lang)

        id = line[:line.find("\t")]
        sentence = line[line.find("\t")+1:].strip()

        d = {}
        for ii in ingrams(line, ngram_length):
            d[ii] = d.get(ii, 0) + 1

        self.features.append(d)
        self.labels.append(self.label_names.index(lang))
        self.id.append("%s-%s" % (lang, id))

    def TrainClassifier(self):
        data = SparseDataSet(self.features, L=self.labels, patternID=self.id)
        print data
        print data.numFeatures
        s = SVM()
        s.train(data)
        return s

class LanguageIdentifier:
    def __init__(self, location, langs, ngram_length=3, num_sents=5000):
        train = TrainingData()
        self.tokenizers = {}
        for ii in langs:
            self.tokenizers[ii] = nltk.data.load('tokenizers/punkt/%s.pickle' % LANG_LOOKUP[ii])
            print location, ii
            filename = glob(location + "/%s*/sentences.txt" % ii)[0]

            sents_seen = 0
            for jj in codecs.open(filename, encoding="utf-8", errors="replace"):
                if num_sents > 0 and sents_seen > num_sents:
                    break
                train.AddInstance(ii, jj, ngram_length)
                sents_seen += 1

        self._labels = train.label_names
        self._classifier = train.TrainClassifier()

    def classify(self, sentence, tokenizer_lang, ngram_length=3):
        features = []
        for ii in self.tokenizers[tokenizer_lang].tokenize(sentence):
            d = {}
            for jj in ingrams(ii, ngram_length):
                d[jj] = d.get(jj, 0) + 1
            features.append(d)
        data = SparseDataSet(features)

        f = FreqDist()
        for ii in [self._labels[self._classifier.classify(data, x)[0]] for x in xrange(len(features))]:
            f.inc(ii)
        return f

if __name__ == "__main__":
    c = LanguageIdentifier("../../data/wortschatz/", ["en","de"], 3, 500)
    pred = c.classify("Ich bin sehr stolz auf dich, mein liebling schatz!  I am very proud of you, my lovely precious!", "de")
    print pred
