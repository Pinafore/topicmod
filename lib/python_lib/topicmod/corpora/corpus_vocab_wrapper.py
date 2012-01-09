# File to read in corpus protocol buffer and give easy access to the various
# vocabularies.

from topicmod.corpora.proto.corpus_pb2 import *


class TermWrapper:

    def __init__(proto_term):
        self.data = proto_term
        self.valid = True
        self.mapping = -1


def parse_proto_vocab(proto_vocab, destination):
    for ii in proto_vocab.terms:
        destination[ii.id] = ii
    return destination


class CorpusVocabWrapper:

    def __init__(self, filename):
        cp = Corpus()
        cp.ParseFromString(open(filename, 'r').read())

        self.tokens = {}
        self.lemmas = {}
        self.pos = {}
        self.synsets = {}

        self.authors = parse_proto_vocab(cp.authors, {})

        for ii in cp.tokens:
            self.tokens[ii.language] = parse_proto_vocab(ii, {})

        for ii in cp.lemmas:
            self.lemmas[ii.language] = parse_proto_vocab(ii, {})

        for ii in cp.pos:
            self.pos[ii.language] = parse_proto_vocab(ii, {})

        for ii in cp.tokens:
            self.synsets[ii.language] = parse_proto_vocab(cp.synsets, {})
