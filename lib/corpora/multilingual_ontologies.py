
from collections import defaultdict

from topicmod.util import flags
from topicmod.util.wordnet import load_wn
from topicmod.ling.dictionary import *
from topicmod.ling.snowball_wrapper import Snowball
from topicmod.corpora.ontology_writer import OntologyWriter
from topicmod.corpora.ontology_writer import orderedTraversal
from topicmod.corpora.ml_vocab import MultilingualVocab

from topicmod.corpora.proto.corpus_pb2 import *

flags.define_int("limit", 250, "How many items in our MuTo matching")
flags.define_bool("dictionary", False, "Use a dictionary")
flags.define_bool("translation", False, "Use translation matrix")
flags.define_bool("greedy_matching", False, "Use a matching from dictionary")
flags.define_bool("wordnet", False, "Use WordNet as scaffold")
flags.define_bool("german", False, "Include German")
flags.define_bool("chinese", False, "Include Chinese")
flags.define_string("output", "", "Where we write ontology")
flags.define_float("trans_cutoff", 0.5, "Min value for using the translation")
flags.define_string("wn_version", "3.0", "Which version of WN we use")
flags.define_string("filter_vocab", "", "Filter entries based on vocabulary")
flags.define_bool("stem", False, "Stem words")
flags.define_bool("id_strings", False, "Add identical strings")


def greedy_german_matching(filter_list, limit, stem):
  stemmer = Snowball()
  translations = defaultdict(set)

  for en, de in DingEntries("en", "de"):
    for ii in en.words():
      for jj in de.words():
        weight = 1.0 / float(len(en.words()))

        for kk in en.words():
          # Make sure both sides are in the vocabulary of interest
          if filter_list.get_id(ENGLISH, kk) != -1 and \
                  filter_list.get_id(GERMAN, jj) != -1:
            if stem:
              translations[stemmer(ENGLISH, kk)].add(stemmer(GERMAN, jj))
            else:
              translations[kk].add(jj)

  # Now that we have all the words, get the weights
  matrix = defaultdict(set)
  for ii in translations:
    weight = 1.0 / float(len(translations[ii]))
    for jj in translations[ii]:
      matrix[weight].add((ii, jj))

  translations = None

  weights = matrix.keys()
  weights.sort(reverse=True)

  right_matched = set([])
  matches = defaultdict(dict)

  for ii in weights:
    print ii, len(matrix[ii]), list(matrix[ii])[:10]
    for left, right in matrix[ii]:
      # Check to see if it's valid
      if not left in matches and not right in right_matched:
        matches[left][GERMAN] = [right]
        matches[left][ENGLISH] = [left]
        right_matched.add(right)

        if len(matches) >= limit:
          return matches
  return matches


def mapSynsetWords(mapping, synset, balanced):
    words = [(ENGLISH, x.name,) for x in synset.lemmas]
    for jj in synset.lemmas:
      for ll in mapping[jj.name]:
        if ll != ENGLISH:
          words += [(ll, x,) for x in mapping[jj.name][ll]]

    # Remove words if they're only English
    if min(x[0] == ENGLISH for x in words) and balanced:
      words = []
    return words


def writeTranslatedWN(mapping, output, balanced=True):
  from nltk.corpus import wordnet_ic
  from nltk.corpus import wordnet as wn
  ic = wordnet_ic.ic("ic-bnc-resnik-add1.dat")

  removed = set([])

  # We gave them hyponym counts, so we don't need to propagate counts
  o = OntologyWriter(output, propagate_counts=False)

  for ii in orderedTraversal(wn, pos='n', reverse_depth=True):
    children = [x.offset for x in ii.hyponyms() + ii.instance_hyponyms() \
                if not x.offset in removed]
    words = mapSynsetWords(mapping, ii, balanced)
    if len(children) == 0 and len(words) == 0:
      removed.add(ii.offset)

  print ("%i synsets removed" % len(removed))
  for ii in orderedTraversal(wn, pos='n'):
    children = [x.offset for x in ii.hyponyms() + ii.instance_hyponyms() \
                if not x.offset in removed]
    hyponym_count = sum(ic['n'][x] for x in children)
    information_contribution = ic['n'][ii.offset] - hyponym_count

    words = mapSynsetWords(mapping, ii, balanced)

    assert information_contribution > 0.0 or information_contribution == 0.0 \
           and len(ii.lemmas) == 0, "Synset %i had no information" % ii.offset
    if len(words) > 0:
      per_word_contribution = information_contribution / float(len(words))
      words = [x + (per_word_contribution,) for x in words]

    # Add synsets if they're not vestigial leaves
    if len(children) > 0 or len(words) > 0:
      o.AddSynset(ii.offset, ii.name, children, words, hyponym_count)
  o.Finalize()


def writeTranslations(mapping, output):
  """
  Takes a dictionary where the primary keys are words in one language and the
  secondary keys are other languages.  Creates a WordNet that's very bushy with
  the primary keys as nodes that are the parents on all the children.
  """

  synset = 0
  o = OntologyWriter(output)
  o.AddSynset(len(mapping), "ROOT", xrange(len(mapping)), [])
  for ii in mapping:
    words = []
    num_words = sum(len(mapping[ii][x]) for x in mapping[ii])
    smoothing = 1.0 / num_words
    for lang in mapping[ii]:
      words += [(lang, x, smoothing) for x in mapping[ii][lang]]

    if synset % 1000 == 0:
      print synset, words
    try:
      o.AddSynset(synset, ii, [], words)
      synset += 1
    except ValueError:
      print "ERROR", ii, words
  print "DONE!", synset, len(mapping)
  assert(synset <= len(mapping))
  o.Finalize()


class Mapping:

  def __init__(self):
    self._store = defaultdict(set)

  def __getitem__(self, item):
    return self._store[item]

  def __iter__(self):
    for ii in self._store:
      yield ii
    return


def filterDictionary(d, filter):
    words_to_remove = set([])
    words_seen = defaultdict(set)

    for ii in d:
      langs = set([])
      for lang in d[ii]:
        init_words = list(d[ii][lang])
        for jj in init_words:
          if filter.get_id(lang, jj) == -1:
            d[ii][lang].remove(jj)
          else:
            words_seen[lang].add(jj)

        if len(d[ii][lang]) > 0:
          langs.add(lang)

      # Don't leave monolingual dictionary entries
      if len(langs) <= 1:
        words_to_remove.add(ii)

    for ii in words_to_remove:
        del d[ii]

    if flags.id_strings:
      # Find all the languages in consideration
      langs = set([])
      for ii in d:
        for jj in d[ii]:
          langs.add(jj)

      # Go through every word
      base_lang = list(langs)[0]
      for ii in filter._reverse[base_lang]:
        # Add a word if it's in every vocab and hasn't been added to mapping
        if all(filter.get_id(x, ii) != -1 for x in langs) and \
              not any(ii in words_seen[x] for x in langs):
          entry_name = "ident-%s" % ii
          d[entry_name][base_lang].add(ii)
          for jj in langs:
            d[entry_name][jj].add(ii)
          print entry_name

    return d


def createDictionary(dictionary, translation, langs, stem):

  d = defaultdict(Mapping)
  stemmer = Snowball()

  if dictionary:
    synset = 0
    if CHINESE in langs:
      print "ADDING CHINESE DICTIONARY"

      # choose dictionary
      if GERMAN in langs:
        chinese_lookup = CedictEntries(language="de")
        target_lang = GERMAN
      else:
        chinese_lookup = CedictEntries(language="en")
        target_lang = ENGLISH

      for ii in chinese_lookup:
        synset += 1
        entry_name = "%i %s" % (synset, ii.name())
        for kk in ii.trans:
          if stem:
            d[entry_name][target_lang].add(stemmer(target_lang, jj))
          else:
            d[entry_name][target_lang].add(kk)  # English synonyms

        if stem:
          d[entry_name][CHINESE].add(stemmer(CHINESE, ii.simplified))
          d[entry_name][CHINESE].add(stemmer(CHINESE, ii.traditional))
        else:
          d[entry_name][CHINESE].add(ii.simplified)
          d[entry_name][CHINESE].add(ii.traditional)

    if GERMAN in langs and not CHINESE in langs:
      assert not ENGLISH in langs, "We don't have a three way dictionary"
      print "ADDING GERMAN DICTIONARY"
      for en, de in DingEntries("en", "de"):
        synset += 1
        entry_name = "%i %s|%s" % (synset, ":".join(en.words()), \
                                     ":".join(de.words()))
        for ii in en.words():
          # Put in all the english synonyms
          if stem:
            d[entry_name][ENGLISH].add(stemmer(ENGLISH, ii))
          else:
            d[entry_name][ENGLISH].add(ii)

        for jj in de.words():
          if stem:
            d[entry_name][GERMAN].add(stemmer(GERMAN, jj))
          else:
            d[entry_name][GERMAN].add(jj)

  if translation:
    if GERMAN in langs:
      for en, de in TranslationProbs("en", "de", flags.trans_cutoff):
        if stem:
          d[en][ENGLISH].add(stemmer(ENGLISH, en))
          d[en][GERMAN].add(stemmer(GERMAN, de))
        else:
          d[en][ENGLISH].add(en)
          d[en][GERMAN].add(de)

    if CHINESE in langs:
      for en, zh in TranslationProbs("en", "zh", flags.trans_cutoff):
        if stem:
          d[en][ENGLISH].add(stemmer(ENGLISH, en))
          d[en][CHINESE].add(stemmer(CHINESE, en))
        else:
          d[en][ENGLISH].add(en)
          d[en][CHINESE].add(zh)

  return d

if __name__ == "__main__":
  flags.InitFlags()
  langs = []
  if flags.german:
    langs.append(GERMAN)
  if flags.chinese:
    langs.append(CHINESE)

  if flags.filter_vocab:
      filter_list = MultilingualVocab(flags.filter_vocab)

  if flags.greedy_matching:
      assert flags.german and not flags.chinese
      mapping = greedy_german_matching(filter_list, flags.limit, flags.stem)
  else:
      mapping = createDictionary(flags.dictionary, flags.translation, langs,
                                 flags.stem)

      if flags.filter_vocab:
          mapping = filterDictionary(mapping, filter_list)

  if flags.wordnet:
    writeTranslatedWN(mapping, flags.output)
  else:
    writeTranslations(mapping, flags.output)
