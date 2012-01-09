
import codecs
import re
import sys
from glob import glob

print sys.version

import nltk
from nltk.tokenize.texttiling import TextTilingTokenizer
from nltk.data import load

punkt = nltk.data.load("nltk:tokenizers/punkt/english.pickle")
punct_regexp = re.compile("[\w\s\\']+\W")

class TexttileWrapper:
    def __init__(self):
        self._tt = TextTilingTokenizer()

    def sentence_array_texttile(self, sentences):
        text = "  \n\n".join(x for x in sentences if len(x) > 0) + "\n\n"
        tok = self._tt.tokenize(text)

        assignments = [0] * len(sentences)

        if tok:
            for ii in xrange(len(sentences)):
                try:
                    assignments[ii] = min(x for x in xrange(len(tok)) if sentences[ii] in tok[x]) + 1
                except ValueError:
                    print("ERROR %i!" % ii)
                    #print(text.encode("ascii", "ignore"))
                    #print(tok)
                    assignments[ii] = 0

        print "**************"
        print assignments

        # Make assignments monotonically increasing
        last_assignment = -1
        assignments_seen = -1
        for ii in xrange(len(assignments)):
            if assignments[ii] != last_assignment:
                assignments_seen += 1
            last_assignment = assignments[ii]
            assignments[ii] = assignments_seen
        print assignments

        return assignments


    def fallback_segmenter(self, text, max_sentence_length = 500,
                           arbitrary_words_per_sent = 30,
                           max_sentences_per_texttile = 15,
                           arbitrary_sentences_per_tile = 6):
        # First, try to segment into sentences with punkt
        sentences = punkt.tokenize(text)

        # If that doesn't work, use a really stupid regexp
        longest_sentence = max(len(x) for x in sentences)
        print ("Longest sentence is %i" % longest_sentence)
        if longest_sentence > 500:
            print "Using regexp sentence breaker"
            sentences = punct_regexp.findall(text)

        # If that still doesn't work, use arbitrary breaks
        if max(len(x) for x in sentences) > 600:
            print "Using ad hoc sentence breaker"
            sentences = []
            words = text.split()
            num_words = len(words)
            for ii in xrange(num_words // arbitrary_words_per_sent + 1):
                sentences.append(" ".join(words[ii * arbitrary_words_per_sent:
                                                min((ii + 1) * arbitrary_words_per_sent,
                                                    num_words)]))

        # Now feed that into texttile
        print(sentences)
        try:
            tile_assignments = self.sentence_array_texttile(sentences)
            tiles = set(tile_assignments)
        except ValueError:
            tile_assignments = None

        # If that doesn't work, split "sentences", however defined, into reasonable
        # sized chunks
        if tile_assignments == None or max(sum(1 for y in tile_assignments if y == x) for x in tiles) > max_sentences_per_texttile:
            tile_assignments = [x // arbitrary_sentences_per_tile for x in xrange(len(sentences))]

        return sentences, tile_assignments

    def fallback_wrapper(self, text):
        sentences, assignments = self.fallback_segmenter(text)

        num_sents = len(sentences)
        tiles = []
        for ii in xrange(max(assignments) + 1):
            tiles.append(" ".join(sentences[x] for x in xrange(num_sents) \
                                    if assignments[x] == ii))
        return tiles

if __name__ == "__main__":
    files = sys.argv[1:]
    expanded_files = set([])
    for ii in files:
        for jj in glob(ii):
            expanded_files.add(jj)

    tt = TexttileWrapper()
    for ii in expanded_files:
        print ii
        text = codecs.open(ii, 'r', 'utf-8').read()
        o = codecs.open("%s.tile" % ii, 'w', 'utf-8')
        for jj in tt.fallback_wrapper(text):
            o.write("%s\n" % jj)
        o.close()
