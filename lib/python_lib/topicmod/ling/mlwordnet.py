from collections import defaultdict
import codecs
from xml.dom import minidom
from glob import glob

import nltk

from nltk.corpus.reader.wordnet import WordNetError

from topicmod.util.wordnet import load_wn
from topicmod.util import flags
from topicmod.util.sets import flatten

flags.define_list("gn_valid_relations", ["hyperonym", "synonym", "near_synonym"], "What relationships we view as equivalent in GermaNet")

kVALID_POS = ['n', 'v', 'a']

# TODO(JBG): Relationships aren't being read; this is just a
# collection of synsets currently.

def find_equiv(pos, word, offset, old_wn, new_wn):
    """
    Given a pos, word, and offset in an old version of wordnet, finds
    the current version.  Uses the sense key when it can, still tries to
    find unambiguous answers when it can't.
    """
    word_matches = new_wn.synsets(word, pos)

    try:
        old_synset = old_wn._synset_from_pos_and_offset(pos, int(offset))
    except WordNetError:
        old_synset = None
    except ValueError:
        old_synset = None
        print "Could not parse integer:", word, offset, pos
    except StopIteration:
        old_synset = None
        print "Could not seek:", word, offset, pos

    if not old_synset:
        if len(word_matches) == 1:
            print "Rescued sense after offset clash:", word_matches[0]
            return word_matches[0]
        else:
            print "Could not find old synset:", pos, offset, word
            return None

    # First, see if the answer is easy and we can look up the key directly
    try:
        new_synset = new_wn.lemma_from_key(old_synset.lemmas[0].key).synset
        return new_synset
    except WordNetError:
        if len(word_matches) == 1:
            print "Rescued sense after missing key:", word_matches, word
            return word_matches[0]

        new_matches = list(flatten(new_wn.synsets(x.name, pos) for x in old_synset.lemmas))
        if len(new_matches) == 1:
            print "Rescued sense after abmiguous old word"
            return new_matches[0]

        intersection = list(set(word_matches) & set(new_matches))
        if len(intersection) == 1:
            print "rescued sense after intersecting"
            return intersection[0]
        return None


class MultiLingSense:
    def __init__(self, word, synset):
        self.word = word
        self._synset = synset

class MultiLingSynset:
    def __init__(self, pos, key):
        self.words = set()
        self.key = key
        self.pos = pos
        self.example = ""
        assert pos in kVALID_POS, "Invalid part of speech: %s" % pos

    def add_word(self, word):
        self.words.add(word)

    def __unicode__(self):
        return u'%s.%s (%s)' % (self.pos, " ".join(self.words), unicode(self.key))

class MultiLingWordNet:
    def __init__(self, lang):
        self._lang = lang
        self._translation = {}
        self._full_lookup = {}
        self._ambig_lookup = {}
        for ii in kVALID_POS:
            self._ambig_lookup[ii] = defaultdict(set)

    def _add(self, synset):
        for ii in synset.words:
            self._ambig_lookup[synset.pos][ii].add(synset)
        if synset in self._full_lookup:
            print "Warning!  Synset already added: ", synset
        self._full_lookup[synset.name] = synset

    def synset_from_en_key(self, key):
        raise NotImplementedError

    def synset_from_key(self, key):
        return self._translation.get(key, None)

    def synsets(self, word, pos=None):
        if pos:
            for ii in self._ambig_lookup[pos][word]:
                yield ii
        else:
            for ii in self._ambig_lookup:
                for jj in self._ambig_lookup[ii][word]:
                    yield jj

class GermaNetSense(MultiLingSense):
    def __init__(self, word, synset, lex):
        MultiLingSense.__init__(self, word, synset)
        self._orth_var = (lex.getAttribute('orthVar') == 'ja')
        self._artificial = (lex.getAttribute('artificial') == 'ja')
        self._proper = (lex.getAttribute('Eigenname') == 'ja')
        self._sense = int(lex.getAttribute('sense'))
        self._statue = lex.getAttribute('status')
        self._style = (lex.getAttribute('stilMarkierung') == 'ja')

class GermaNetSynset(MultiLingSynset):
    def __init__(self, synset_from_minidom):
        key = synset_from_minidom.getAttribute('id')
        pos = synset_from_minidom.getAttribute('wordClass')[0]
        MultiLingSynset.__init__(self, pos, key)

        for lex in synset_from_minidom.getElementsByTagName('lexUnit'):
            for word in lex.getElementsByTagName('orthForm'):
                self.add_word(GermaNetSense(word.firstChild.toxml(), self, lex))

        example = synset_from_minidom.getElementsByTagName('example')
        if example:
            self.example = example[0].firstChild.nodeValue
        else:
            self.example = None

class GermaNet(MultiLingWordNet):
    def __init__(self, directory=None):
        if not directory:
            directory = find_data("germanet")

        MultiLingWordNet.__init__(self, "de")

        print directory + "/xml/*.xml"
        for ii in glob(directory + "/xml/*.*.xml"):
            print "Reading ", ii
            self.parse_xml(ii)

    def parse_xml(self, filename):
        xmlfile = minidom.parse(filename)
        synlist = xmlfile.getElementsByTagName('synset')
        for synset in synlist:
            self._add(GermaNetSynset(synset))

class MultilingMapping:
    def __init__(self):
        self._mapping = {}

    def related_words(self, synset, lang=""):
        if lang:
            if lang == "en":
                for jj in synset.lemmas:
                    yield jj.name.lower()
            else:
                for jj in self._mapping[lang][synset.name]:
                    yield jj
        else:
            for ii in self._mapping:
                for jj in self._mapping[ii][synset.name]:
                    yield jj

    def add_language(self, lang):
        if lang == "de":
            self.read_german()

    def read_german(self, directory = "../../data/germanet/"):
        old_wn = load_wn("2.0")
        new_wn = load_wn("3.0")

        self._mapping["de"] = defaultdict(set)
        for ii in glob(directory + "/ILI*"):
            print "Reading mapping from ", ii
            for jj in codecs.open(ii, 'r', "latin-1"):
                fields = jj.split()
                word = fields[0]
                if word.startswith("$"):
                    print "Spurious symbol: %s" % word.encode("ascii", "ignore")
                    word = word.replace("$", "")
                if word.startswith("?"):
                    print "Spurious symbol: %s" % word.encode("ascii", "ignore")
                    word = word.replace("?", "")

                fields = fields[4:]
                fields.reverse()

                while fields:
                    try:
                        link_type = fields.pop()
                        eng_word = fields.pop()
                        eng_sense = fields.pop()
                        synset = fields.pop()
                    except IndexError:
                        print "Pop error:", jj.encode("ascii", 'ignore'), fields
                        break

                    if synset.startswith("ENG20"):
                        vers, offset, pos = synset.split("-")
                        assert vers == "ENG20", "Wrong version of WordNet: %s" % vers
                    else:
                        if "-" in synset:
                            offset, pos = synset.split("-")
                        else:
                            continue

                    new_synset = find_equiv(pos, eng_word, offset, old_wn, new_wn)
                    if new_synset and link_type in flags.gn_valid_relations:
                        self._mapping["de"][new_synset.name].add(word.lower())



# Load all the languages we have as a test

if __name__ == "__main__":
    flags.InitFlags()

    #gn = GermaNet()
    mapping = MultilingMapping()
    mapping.read_german()

    wn = load_wn()
    print [list(mapping.related_words(x)) for x in wn.synsets("dog")]
