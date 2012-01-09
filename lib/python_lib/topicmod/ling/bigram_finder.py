
from string import punctuation
from collections import defaultdict

from scipy import zeros
from scipy.stats import chisquare

from nltk.util import ibigrams

from topicmod.ling.stop import StopWords

def text_to_bigram(text, bigrams, normalize):
    for ii, ww in split_bigrams_with_index(text, bigrams, normalize):
        yield ww

def iterable_to_bigram(iterable, bigrams, normalize):
    for ii, ww in iterable_to_bigram_offset(iterable, bigrams, normalize):
        yield ww

def iterable_to_bigram_offset(iterable, bigrams, normalize):
    previous_words = []
    last_word = ""
    last_position = -1
    stop = 0
    for ii in iterable:
        current = normalize(ii)
        if current:
            if (last_word, current) in bigrams:
                previous_words.append(current)
            else:
                if previous_words:
                    ngram = "_".join(previous_words)
                    res = (last_position, ngram)
                    yield res
                    
                previous_words = [current]

            last_word = current
            last_position = stop
        stop += 1

    if previous_words:
        yield (last_position, "_".join(previous_words))    

def text_to_bigram_offset(text, bigrams, normalize):
    for ii in index_bigrams_from_iterable(text.split()):
        yield ii

class BigramFinder:
    def __init__(self, additional_stop = [], remove_stop = [], 
                 min_unigram = 10, min_ngram = 5,
                 exclude=[], language="english"):

        print("Loading stopwords for %s" % language)
        self._stopwords = StopWords()[language] | set(additional_stop)
        for ii in remove_stop:
            self._stopwords.remove(ii)

        self._exclude_from_bigram = set(exclude)

        self._min_uni = min_unigram
        self._min_ngram = min_ngram

        self._dual = defaultdict(int)
        self._left = defaultdict(int)
        self._right = defaultdict(int)

        self._unigram = defaultdict(int)

        self._invalid_chars = set(punctuation)

    def normalize_word(self, word):
        word = word.lower()
        reduced = "".join(x for x in word if not x in self._invalid_chars)
        if len(reduced) > 1 and not reduced in self._stopwords:
            return reduced
        else:
            return ""

    def score(self, bigram, array=None):
        if not array:
            array = zeros((2,2))

        left, right = bigram

        if any(x in self._exclude_from_bigram for x in bigram):
            return 0.0

        array[0,0] = self._dual[bigram]
        array[0,1] = self._left[left]
        array[1,0] = self._right[right]
        either = array[1,0] + array[0,1] - array[0,0]
        array[1,1] = self._total - either

        stat, p = chisquare(array)
        return p[0]

    def set_counts(self, counts):
        for ii in counts:
            if counts[ii] >= self._min_uni:
                self._unigram[self.normalize_word(ii)] += counts[ii]

    def create_counts(self, tokens):
        for word in tokens:
            reduced = self.normalize_word(word)
            if reduced:
                self._unigram[reduced] += 1

        # now cull low counts
        to_delete = set()
        for ww in self._unigram:
            if self._unigram[ww] < self._min_uni:
                to_delete.add(ww)

        for ii in to_delete:
            del self._unigram[ii]

    def add_ngram_counts(self, tokens):
        for ngram in ibigrams(tokens):
            if all(x in self._unigram for x in ngram):
                self._dual[ngram] += 1        

    def find_ngrams(self, tokens = []):
        self.add_ngram_counts(tokens)

        to_delete = set()
        for ngram in self._dual:
            if self._dual[ngram] < self._min_ngram:
                to_delete.add(ngram)

        for ngram in to_delete:
            del self._dual[ngram]

        self._total = 0
        for ll, rr in self._dual:
            count = self._dual[(ll, rr)]
            self._total += count
            self._left[ll] += count
            self._right[rr] += count

    def real_ngrams(self, cutoff):
        d = {}
        for ii in self._dual:
            score = self.score(ii)
            if score > cutoff:
                d[ii] = self._dual[ii]
        return d
        
    def print_ngrams(self, limit = 10):
        num_tokens = 0
        for ii in self._dual:
            if num_tokens > limit:
                break
            print(ii, self._left[ii[0]], self._right[ii[1]], self._dual[ii], self.score(ii))
            num_tokens += 1
                        

    def print_unigrams(self, limit=10):

        num_tokens = 0
        for ii in self._unigram:
            if num_tokens > limit:
                break
            print(ii, self._unigram[ii])
            num_tokens += 1
