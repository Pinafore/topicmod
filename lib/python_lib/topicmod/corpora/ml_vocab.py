from collections import defaultdict
from string import strip
import codecs

from topicmod.corpora.proto.corpus_pb2 import *

kLANGUAGE_ID = {0: ENGLISH, 1: GERMAN, 2: CHINESE}


def extract_mapping(vocab, output_file=None, existing_mapping={}):
    mapping = existing_mapping
    num_terms = -1
    for ii in vocab.terms:
        if ii.id in mapping:
            assert mapping[ii.id] == ii.original, "Inconsistent authors"
        mapping[ii.id] = ii.original
        
        num_terms = max(ii.id, num_terms)

    if output_file:
        o = codecs.open(output_file, 'w', 'utf-8')
        for ii in xrange(num_terms):
            o.write(u"%s\n" % mapping[ii])

    return mapping



class Vocab:

    def __init__(self, filename=None, filter_language=ENGLISH):
        self.word_to_int_ = {}
        self.int_to_word_ = {}
        self.int_to_unicode_ = {}
        index = -1

        if filename:
            infile = codecs.open(filename, encoding='utf-8', errors="replace")

            while True:
                term = infile.readline()
                if not term:
                    break

                if "\t" in term:
                    language, term = term.split("\t")
                    if filter_language != int(language):
                        continue
                    else:
                        index += 1


                # This is for comments in the vocab file; we ignore everything
                # after the comment
                if "#" in term:
                    term = term.split("#")[0]
                term = strip(term)

                # If our vocabulary term has different expressions,
                # we want all of them to map to this index
                for jj in term.split(":"):
                    self.word_to_int_[jj] = index
                    self.int_to_word_[index] = term.encode("ascii", 'replace')
                    self.int_to_unicode_[index] = term.split(":")[-1]

            print "Read", index, "items from", filename, "."

    def AddWord(self, word):
        if not word in self.word_to_int_:
            index = len(self.int_to_word_)
            self.word_to_int_[word] = index
            self.int_to_word_[index] = word
            self.int_to_unicode_[index] = word

    def PrintLine(self, line):
        s = ""
        for ii in line:
            s += "%s:%s " % (self.int_to_word_[ii], line[ii])
        s += "\n"
        return s

    def __getitem__(self, val):
        if isinstance(val, int):
            return self.int_to_word_[val]
        else:
            return self.word_to_int_[val]

    def __contains__(self, val):
        return val in self.word_to_int_

    def write(self, file):
        l = [(ii, self.int_to_unicode_[ii]) for ii in self.int_to_unicode_]
        l.sort()

        o = codecs.open(file, 'w', encoding='utf-8')
        for ii in l:
            o.write(ii[1] + "\n")

        print "Wrote", len(l), " lines to vocab file ", file

    def __len__(self):
        return len(self.int_to_word_)


class MultilingualVocab:

    def __init__(self, filename=""):
        self._lookup = defaultdict(dict)
        self._reverse = defaultdict(dict)

        if filename:
            counts = defaultdict(int)
            infile = codecs.open(filename, encoding="utf-8")
            for ii in infile:
                lang, term = ii.split("\t")
                term = term.strip()
                lang = kLANGUAGE_ID[int(lang)]
                self.set(lang, counts[lang], term)
                counts[lang] += 1

    def set(self, lang, id, word):
        self._lookup[lang][id] = word
        self._reverse[lang][word] = id

    def get_word(self, lang, id):
        return self._lookup[lang].get(id, "")

    def get_id(self, lang, word):
        return self._reverse[lang].get(word, -1)
