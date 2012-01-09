from topicmod.corpora.proto.corpus_pb2 import Corpus
from topicmod.util import flags

flags.define_glob("corpus_parts", None, "Where we look for vocab")
flags.define_string("output", None, "Where we write the mapping")

if __name__ == "__main__":
    flags.InitFlags()

    mapping = {}

    for ii in flags.corpus_parts:
        print ii
        cp = Corpus()
        cp.ParseFromString(open(ii, 'r').read())

        for ii in cp.authors.terms:
            if ii.id in mapping:
                assert mapping[ii.id] == ii.original
            mapping[ii.id] = ii.original

    o = open(flags.output, 'w')
    for ii in xrange(max(mapping)):
        o.write("%s\n" % mapping[ii])
