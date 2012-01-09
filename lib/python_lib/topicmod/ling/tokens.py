#!/usr/bin/python
#-*- coding: latin-1 -*-

import nltk.data

from string import lower, punctuation
from collections import defaultdict

from nltk.stem import PorterStemmer
from nltk.corpus import wordnet as wn

from topicmod.ling.stop import StopWords

porter = PorterStemmer()
porter_stem = porter.stem

def morphy_stem(s):
    first_try = wn.morphy(s)
    if first_try:
        return first_try
    else:
        return s

stopwords = StopWords()
english_stopwords = stopwords.load("english")
arabic_stopwords = stopwords.load("arabic")

tokenizer = nltk.data.load('tokenizers/punkt/english.pickle')

def default_tokenize(raw_text):
    sentences = tokenizer.tokenize(raw_text)
    for ii in sentences:
        for jj in nltk.word_tokenize(ii):
            yield jj

def map_token_func(raw_token,
                   min_length = 3,
                   stopwords = english_stopwords):
    raw_token = raw_token.lower()
    if len(raw_token) < min_length or raw_token in stopwords:
        return ""
    else:
        return stem(raw_token)

def tokens(raw_text, min_length = 3, stopwords = english_stopwords, stem_words="morphy", exclude_punctuation = True, tokenize_function=default_tokenize):
    raw_text = raw_text.replace("\n", " ")
    tokens = map(lower, tokenize_function(raw_text))
    #print tokens[:10]
    tokens = filter(lambda x: len(x) >= min_length, tokens)
    #print tokens[:10]
    tokens = filter(lambda x: not x in stopwords, tokens)
    if exclude_punctuation:
        tokens = filter(lambda x: not x[0] in punctuation, tokens)
    if stem_words == "porter":
        tokens = map(porter_stem, tokens)
    elif stem_words == "morphy":
        tokens = map(morphy_stem, tokens)
    else:
        raise NotImplementedError, "%s stemming isn't supported" % stem_words

    return tokens

def tokens_arabic(raw_text, min_length = 1, stopwords = arabic_stopwords):
    raw_text = raw_text.replace("\n", " ")

    # Preprocessing has taken care of the hard stuff for us
    tokens = raw_text.split()
    # print tokens[:10]
    tokens = filter(lambda x: len(x) >= min_length, tokens)
    # print tokens[:10]
    tokens = filter(lambda x: not x in stopwords, tokens)
    # print tokens[:10]

    return tokens

def counts(raw_text, vocab=None, token_func=tokens):
    d = defaultdict(int)
    for ii in token_func(raw_text):
        key = ii
        if vocab:
            if ii in vocab:
                key = vocab[ii]
            else:
                continue
        d[key] += 1
    return d

def german_unicode_to_ascii(german):
    german = german.replace(u'ü', u'ue')
    german = german.replace(u'ö', u'oe')
    german = german.replace(u'ä', u'ae')
    german = german.replace(u'ß', u'ss')
    return german.encode("ascii", "ignore")
