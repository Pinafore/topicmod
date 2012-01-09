import os
import gzip

from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.util import flags

# Input
flags.define_string("doc_filter", None, "Files to filter out")
flags.define_string("vocab", None, "The file that defines the vocab")
flags.define_string("state_file", None, \
                    "The state file that create the corpus")

# Output
flags.define_string("state_output", None, "Where we write state file")
flags.define_string("corpus_output_path", None, "Where we write the corpus")
flags.define_string("corpus_name", "NIH", "Name of the corpus")

# Options
flags.define_int("docs_per_index", 5000, "Number of docs per section")
flags.define_int("doc_limit", -1, "Cap on number of documents")


class MalletAssignment:

  def __init__(self, line, debug=False):
    if debug:
      for ii in xrange(len(line.split())):
        print ii, line.split()[ii]
    self.doc, foo, self.index, self.term_id, self.term, self.assignment = \
      line.split()
    self.doc = int(self.doc)
    self.index = int(self.index)
    self.term_id = int(self.term_id)
    self.assignment = int(self.assignment)


class MalletStateBuffer:

  def __init__(self, filename):
    self.infile = gzip.GzipFile(filename)
    self.current_doc = -1
    self.buffer = []

  def __iter__(self):
    for ii in self.infile:
      if ii.startswith("#"):
        continue
      line = MalletAssignment(ii)

      if line.doc != self.current_doc and self.buffer:
        yield self.buffer
        self.buffer = []
      self.current_doc = line.doc

      self.buffer.append(line)
    yield self.buffer


def write_proto(filename, proto):
  f = open(filename, "wb")
  f.write(proto.SerializeToString())
  f.close()


def state_line(mallet_doc):
  return "%i %s\n" % (len(mallet_doc), " ".join(str(x.assignment) \
                      for x in mallet_doc))


def translate_mallet_doc(mallet_doc, vocab):
  d = Document()
  s = d.sentences.add()
  d.id = mallet_doc[0].doc
  for ii in mallet_doc:
    assert d.id == ii.doc, "Inconsistent document assignment"
    w = s.words.add()
    w.token = ii.term_id
    w.lemma = ii.term_id

    assert(ii.term_id in vocab), "No entry for %s (%i)" % (ii.term, w.term_id)
    assert(ii.term == vocab[ii.term_id]), "%s %s mismatch" % \
        (ii.term, vocab[ii.term_id])
  return d


def initialize_corpus_vocab(input_file):
  """
  Reads in the vocab file and creates a new corpus
  """
  c = Corpus()
  voc = c.tokens.add()
  voc.language = ENGLISH
  lookup = {}

  line_num = 0
  for ii in open(input_file):
    term = ii.split("\t")[1].strip()

    word = voc.terms.add()
    word.id = line_num
    word.original = term
    word.ascii = term.encode("ascii", "replace")
    word.frequency = 0
    word.stop_word = False
    lookup[line_num] = term

    line_num += 1
  return c, lookup


if __name__ == "__main__":
  flags.InitFlags()

  doc_num = 0
  index_num = -1
  record_num = 0

  # Create the doc filter if we need it
  doc_filter = {}
  if flags.doc_filter:
    for ii in open(flags.doc_filter):
      if ii.startswith("Doc#"): # Ignore header
        continue
      fields = ii.split("\t")
      doc_filter[int(fields[0])] = fields[-1].strip()

  o_state = open("%s.topic_assignments" % flags.state_output, 'w')
  o_path = open("%s.path_assignments" % flags.state_output, 'w')
  o_doc = open("%s.doc_id" % flags.state_output, 'w')

  corpus, vocab = initialize_corpus_vocab(flags.vocab)
  for doc in MalletStateBuffer(flags.state_file):
    record_num += 1
    doc_proto = translate_mallet_doc(doc, vocab)

    if doc_filter and not doc_proto.id in doc_filter:
      continue

    # Divide up the corpus into smaller chunks
    if doc_num % flags.docs_per_index == 0:
      # Write the old corpus if it exists
      if index_num >= 0:
        write_proto("%s/%s_%04i.index" % (flags.corpus_output_path,
                                          flags.corpus_name, index_num),
                    corpus)

        corpus, vocab = initialize_corpus_vocab(flags.vocab)
      index_num += 1

      # Create a place to put down new documents
      directory = "%s/%s_%i" % (flags.corpus_output_path,
                                flags.corpus_name,
                                index_num)
      if not os.path.exists(directory):
        os.mkdir(directory)
      print index_num, doc_num, record_num

    doc_filename = "%s_%i/%i" % (flags.corpus_name,
                                 index_num, doc_num)

    if doc_filter:
      doc_proto.title = doc_filter[doc_proto.id]

    corpus.doc_filenames.append(doc_filename)

    write_proto("%s/%s" % (flags.corpus_output_path, doc_filename), doc_proto)
    o_state.write(state_line(doc))
    o_path.write("%i %s\n" % (len(doc), " ".join(["0"] * len(doc))))

    # Format of doc line is language, space, id, tab,
    #                       title(which we lack), newline
    o_doc.write("%i %i\t\n" % (doc_proto.language, doc_proto.id))
    doc_num += 1

    if flags.doc_limit > 0 and doc_num > flags.doc_limit:
      break

  write_proto("%s/%s_%i.index" % (flags.corpus_output_path,
                                  flags.corpus_name, index_num),
              corpus)

  o_state.close()
