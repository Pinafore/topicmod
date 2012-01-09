
from nltk.corpus import reader
from topicmod.corpora.corpus_reader import *
from proto.corpus_pb2 import *
from glob import glob


class EuroparlDocument(DocumentReader):

    def __init__(self, filename, language, chapter, sentences):
        self.internal_id = filename
        self.chapter = chapter
        self._sentences = sentences
        DocumentReader.__init__(self, "", language)

    def sentences(self):
        for ii in self._sentences:
            yield [unicode(x, 'utf-8').lower() for x in ii]

    def tokens(self):
        for ii in self._sentences:
            for jj in ii:
                yield unicode(jj, 'utf-8').lower()

    def proto(self, num, language, authors, token_vocab, df, lemma_vocab,
              pos, synsets, stemmer):
        d = Document()
        assert language == self.lang

        # Use the given ID if we have it
        d.id = num

        d.language = language
        d.author = authors[self.author]
        title = self.internal_id.split("/")[-1].split(".")[0]
        d.title = "%s-%i" % (title, self.chapter)

        tf_token = nltk.FreqDist()
        for ii in self.sentences():
            for jj in ii:
                tf_token.inc(jj)

        for ii in self.sentences():
            # Maybe split into sentences at some point?
            s = d.sentences.add()
            for jj in ii:
                w = s.words.add()
                w.token = token_vocab[jj]
                w.lemma = lemma_vocab[stemmer(language, jj)]
                w.tfidf = df.compute_tfidf(jj, tf_token.freq(jj))

        return d

    def raw(self):
        raise NotImplementedError


class DocAlignedReader(CorpusReader):

    def __init__(self, base, doc_limit=-1):
        # Maps universal alignment to filenames
        self._doc_alignment = defaultdict(dict)
        CorpusReader.__init__(self, base, doc_limit)

    def lang_iter(self, lang):
        file_list = list(self._doc_alignment.keys())
        file_list.sort()
        random.seed(0)
        random.shuffle(file_list)

        doc_num = 0
        required_langs = set([])

        # Get a list of all languages that we need to support
        print("Required languages ...")
        for ff in file_list:
            for jj in self._doc_alignment[ff]:
                required_langs.add(jj)
        print(" ... %s.  DONE." % str(required_langs))

        for ff in file_list:
            # Make sure that this file exists for all needed languages
            if not min(x in self._doc_alignment[ff] for x in required_langs):
                print "Doc missing", ff
                continue

            filename = self._doc_alignment[ff][lang]

            for dd in self.doc_factory(lang, filename):
                if self._doc_limit > 0 and doc_num >= self._doc_limit:
                    break
                yield dd
                doc_num += 1


class EuroparlCorpus(DocAlignedReader):

    def __init__(self, base, doc_limit=-1):
        self._europarl = reader.plaintext.EuroparlCorpusReader(".", [])
        DocAlignedReader.__init__(self, base, doc_limit)

    def add_language(self, pattern, language):
        search = self._file_base + pattern
        print("Searching for %s" % search)
        for ii in glob(search):
            self._files[language].add(ii)

            universal_name = ii.split("/")[-1].split(".")[0]

            self._doc_alignment[universal_name][language] = ii

  def doc_factory(self, lang, filename):
    chap = 0
    try:
      for ii in self._europarl.chapters(filename):
        yield EuroparlDocument(filename, lang, chap, ii)
        chap += 1
    except AssertionError:
      print "Error with file %s" % filename
