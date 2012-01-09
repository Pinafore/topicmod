
import codecs
import re
from glob import glob
from gzip import GzipFile
from string import printable
from string import lowercase

parens = re.compile("\s*\([^\)]*\)\s*")
cedict_delimiter = re.compile(",|/")


class CedictEntry:

    def __init__(self, line):
        pos = line.find("] /")
        chinese = line[:pos + 1]
        self.traditional, self.simplified, self.pinyin = chinese.split(" ", 2)
        self.pinyin = self.pinyin[1:-1]

        # Remove explanations
        trans = parens.sub(" ", line[pos + 3:]).lower()
        # Separate multiple translations
        trans = [x.strip() for x in cedict_delimiter.split(trans)]
        # Remove empty strings
        trans = [x for x in trans if x and min(y in printable for y in x)]

        for ii in xrange(len(trans)):
            if trans[ii].startswith("a ") or trans[ii].startswith("the ") or \
                   trans[ii].startswith("an ") or trans[ii].startswith("to "):
                #print "Replacing ", trans[ii],
                trans[ii] = trans[ii][trans[ii].find(" ") + 1:]
                #print trans[ii]
        self.trans = trans

    def name(self):
        return u"%s|%s" % (u":".join(self.trans), self.pinyin)

    def display(self):
        print self.traditional
        print self.simplified
        print self.pinyin
        print self.trans


def CedictEntries(filename="../../data/dictionaries/", language="en"):
    for ii in codecs.open(filename + "zh_%s.u8" % language, encoding="utf-8"):
        if not ii.startswith("#"):  # Comment lines
            yield CedictEntry(ii)
    return

re_whitespace = re.compile("\s")
re_annotations = re.compile("[\[\{][^\]\}]*[\}\]]")
re_extra = re.compile("\([^\)]*\)")


class DingWord:

    def __init__(self, line):
        line = line.split("|")

        self.singular = []
        self.plural = []
        self.singular_annotations = []
        self.plural_annotations = []

        if len(line) >= 1:
            self.singular, self.singular_annotations = self.cleanLine(line[0])

        if len(line) >= 2:
            self.plural, self.plural_annotations = self.cleanLine(line[1])

    def __str__(self):
        return str(self.singular)

    def cleanLine(self, segment, remove_infinitive=True):
        annotations = re.findall(re_annotations, segment)
        annotations = map(lambda x: x[1:-1], annotations)

        segment = re_extra.sub("", segment)
        segment = re_annotations.sub("", segment)
        segment = re_whitespace.sub(" ", segment)
        segment = [x.strip() for x in segment.split(";")]

        if remove_infinitive:
            for ii in xrange(len(segment)):
                if segment[ii].startswith("to ") and len(segment[ii]) > 4:
                    segment[ii] = segment[ii][3:]
        return segment, annotations

    def words(self):
        return self.singular + self.plural


def DingEntries(source, target, base="../../data/dictionaries/",
                lower_case=True):
    filename = source + "_" + target + ".ding"
    rev_filename = target + "_" + source + ".ding"

    rev_filename = glob(base + "/" + rev_filename)
    filename = glob(base + "/" + filename)

    # If the order requested doesn't match the file, we'll have to reverse
    # the dictionary
    reverseLookup = (len(rev_filename) != 0)
    filename = (filename + rev_filename)[0]

    for line in codecs.open(filename, 'r', encoding='utf-8'):
            # comments begin with #
        if line[0] == "#":
            continue
        # valid lines must have two words
        if "::" in line:
            try:
                if lower_case:
                    line = line.lower()
                    left, right = line.split("::")
            except ValueError:
                print "BAD LINE:", line
                continue

            # There are spurious characters at the end of lines in the
            # original files; if this changes, this should be changed
            right = right[:-1]

            left = DingWord(left)
            right = DingWord(right)

            if reverseLookup:
                yield right, left
            else:
                yield lef, right
    return


def TranslationProbs(lang1, lang2, cutoff, location="../../data/translation"):
    filename = "%s/lex.%s.%s" % (location, lang1, lang2)

    for ii in GzipFile(filename):
        word1, word2, weight = ii.decode("utf-8").lower().split()
        weight = float(weight)
        if weight >= cutoff and word1 != "null" and \
               min(x in lowercase for x in word1):
            yield (word1, word2)
