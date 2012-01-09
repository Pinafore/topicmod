
from topicmod.corpora.amazon import *
from proto.corpus_pb2 import *


class PangLeeMovie(AmazonDocument):

    def __init__(self, lang, line, author, rating, id_tag):
        self.lang = lang
        self._raw = line
        self.author = author
        self._rating = rating
        self._id = int(id_tag)

    def language(self):
        return self.lang


class PangLeeMovieCorpus(CorpusReader):

    def add_language(self, pattern, response_pattern, language=ENGLISH):
        search = self._file_base + pattern
        print("Looking for %s" % search)
        for ii in glob(search):
            self._files[language].add(ii)

        self._response = response_pattern

    def doc_factory(self, lang, filename):
        content_file = codecs.open(filename, 'r', encoding='utf-8')

        author = filename.split(".")[-1]

        id_lines = codecs.open(filename.replace("subj.", "id."),
                               'r', encoding='utf-8').readlines()
        rating_lines = codecs.open(filename.replace("subj.", "%s."
                                                    % self._response),
                                   'r', encoding='utf-8').readlines()

        # Create offsets
        lines = list()
        line_num = 0
        while True:
            text = content_file.readline()
            if not text:
                break
            lines.append((text, float(rating_lines[line_num]),
                       int(id_lines[line_num])))
            line_num += 1

        random.seed(0)
        random.shuffle(lines)

        for text, rating, title in lines:
            yield PangLeeMovie(lang, text.lower(), author, rating, title)
        return
