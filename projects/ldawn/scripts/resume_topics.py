from collections import defaultdict
from glob import glob
import os
import shutil
import re

from topicmod.util import flags
from topicmod.util.sets import write_pickle, read_pickle
from topicmod.corpora.proto.corpus_pb2 import *
from topicmod.corpora.proto.wordnet_file_pb2 import *

# Faster version of resume_topics_update.py
# clear_doc_topics.py can only perform doc strategy
# clear_topics.py can perform both doc and term strategy
# For term strategy: make sure the word index in generated map is consistent
# with the model.topic_assignment file


# Read in constraints
def get_constraints_from_wn(wn_files):

  cons_dict = defaultdict()
  cl_index = set()
  nl_in_index = set()
  cons_set = set()

  for ii in wn_files:
    wnfile = open(ii, 'rb')
    wn = WordNetFile()
    wn.ParseFromString(wnfile.read())

    for jj in wn.synsets:
      if jj.key.startswith("ML_") or (jj.key.startswith("NL_")
                   and (not jj.key.startswith("NL_REMAINED_"))
                   and (not jj.key.startswith("NL_IN_"))):
        cons = []
        for kk in jj.words:
          cons.append(kk.term_str)
          cons_set.add(kk.term_str)
        # print ii, "CONSTRAINT FOUND:", jj.key
        cons_dict[jj.offset] = cons
      elif jj.key.startswith("CL_"):
        cons_dict[jj.offset] = jj.children_offsets
        cl_index.add(jj.offset)
        # print ii, "CONSTRAINT FOUND:", jj.key
      elif jj.key.startswith("NL_IN_"):
        cons_dict[jj.offset] = jj.children_offsets
        nl_in_index.add(jj.offset)

  ## Get links for split ################################################
  # Add cannot links: Notice we only get the direct children of cannot links
  cn_list = []
  for cl in cl_index:
    cl_children = cons_dict[cl]
    cons = []
    for child in cl_children:
      if child in nl_in_index:
        nl_in_cons = []
        nl_in_children = cons_dict[child]
        for nl_in_child in nl_in_children:
          # nl_in_cons.append(cons_dict[nl_in_child])
          nl_in_cons += cons_dict[nl_in_child]
          cons_dict[nl_in_child] = -1
        cons.append(nl_in_cons)
      else:
        cons.append(cons_dict[child])

      cons_dict[child] = -1

    cons_dict[cl] = -1
    cn_list.append(cons)

  ## Add must links that have not been merged to cannot link
  ml_set = set()
  for index in cons_dict.keys():
    if cons_dict[index] != -1:
      ml_set |= set(cons_dict[index])

  cons_list = []
  cons_list.append(cn_list)
  cons_list.append(ml_set)
  #######################################################################

  return cons_list, cons_set


# Read in constraints
def get_constraints(cons_files):
  cons_set = set()

  # If no constraint file was specified, give an empty constraint
  if cons_files == "":
    return cons_set

  infile = open(cons_files, 'r')
  for line in infile:
    line = line.strip()
    ww = line.split('\t')
    tmp = map(lambda x: x.lower(), ww[1:])
    print tmp
    cons_set |= set(tmp)
  return cons_set


def build_index(corpus_dir, maps_file):
  index = defaultdict()

  num_docs = 0
  read_voc = 0
  for ii in glob("%s/*.index" % corpus_dir):
    inputfile = open(ii, 'rb')
    protocorpus = Corpus()
    protocorpus.ParseFromString(inputfile.read())

    if read_voc == 0:
      # assume that vocab in each index file is the same, so
      # we just need to load it once and initialize index once
      # or it is too slow
      read_voc = 1
      vocab = {}
      for lang in protocorpus.tokens:
        for ii in lang.terms:
          vocab[ii.id] = ii.original
          index[ii.original] = defaultdict(set)

    for dd in protocorpus.doc_filenames:
      num_docs += 1

      docfile = open("%s/%s" % (corpus_dir, dd), 'rb')
      doc = Document()
      doc.ParseFromString(docfile.read())

      if num_docs % 250 == 0:
        print doc.id, doc.title, len(index), " vocab seen"

      word_index = 0
      for jj in doc.sentences:
        for kk in jj.words:
          w = vocab[kk.token]
          index[w][doc.id].add(word_index)
          word_index += 1

    if flags.doc_limit > 0 and num_docs > flags.doc_limit:
      break

  write_pickle(index, maps_file)

  return index


def clear_assignments(input_base, output_base, index,
                      constraint_words, option):

  # Get a set of all documents we need to clear
  relevant_docs = defaultdict(set)

  # print "Constraint words:", constraint_words
  print "Index looks like:", index.keys()[:10]
  for ii in constraint_words:
    for jj in index[ii].keys():
      relevant_docs[jj].add(ii)

  topic_assignments_in = open("%s.topic_assignments" % input_base)
  path_assignments_in = open("%s.path_assignments" % input_base)
  docs_in = open("%s.doc_id" % input_base)

  topic_assignments_out = open("%s.topic_assignments" % output_base, 'w')
  path_assignments_out = open("%s.path_assignments" % output_base, 'w')
  docs_out = open("%s.doc_id" % output_base, 'w')

  for topic_assign, path_assign, name in zip(topic_assignments_in,
                                    path_assignments_in, docs_in):
    docs_out.write(name)
    name = int(name.split()[1])
    if name in relevant_docs.keys():
      if option == 1:
        num_words = len(topic_assign.split()) - 1
        topic_assignments_out.write("%i %s\n" % \
               (num_words, " ".join(["-1"] * num_words)))

        num_words = len(path_assign.split()) - 1
        path_assignments_out.write("%i %s\n" % \
               (num_words, " ".join(["-1"] * num_words)))
      else:
        # option == 0
        topics = topic_assign.split()
        path = path_assign.split()
        for w in relevant_docs[name]:
          for win in index[w][name]:
          # int(j) + 1, because the first item each line
          # is the number of words in that document
            win = int(win) + 1
            topics[win] = '-1'
            path[win] = '-1'
        topic_assignments_out.write("%s\n" % (" ".join(topics)))
        path_assignments_out.write("%s\n" % (" ".join(topics)))
    else:
      topic_assignments_out.write(topic_assign)
      path_assignments_out.write(path_assign)

  topic_assignments_in.close()
  topic_assignments_out.close()
  path_assignments_in.close()
  path_assignments_out.close()
  docs_in.close()
  docs_out.close()


def split_assignments(input_base, output_base, index,
                      constraint_list, option, num_topics):
  # Get a set of all documents we need to clear
  relevant_docs = defaultdict(set)

  # print "Constraint words:", constraint_list
  print "Index looks like:", index.keys()[:10]

  # split
  cn_list = constraint_list[0]
  new_topics = num_topics - 1
  new_topics_index = defaultdict()
  for cons in cn_list:
    # keep the topic assignment of the first word set in each cannot link
    # assign the word sets to a new topic
    for ii in range(1, len(cons)):
      new_topics += 1
      new_topics_flag = 0
      for w in cons[ii]:
        # assert w not in new_topics_index.keys(),
        # "Word %s is in more than one path!" % w
        if w not in new_topics_index.keys():
          new_topics_flag = 1
          new_topics_index[w] = new_topics
        else:
          # if one word appears in more than one path, we simply
          # unassign this word and throw out a warning
          print "Warning: word %s is in more than one path!" % w
          new_topics_index[w] = -1
        for jj in index[w].keys():
          relevant_docs[jj].add(w)
      if new_topics_flag == 0:
        new_topics -= 1

  # clear
  ml_set = constraint_list[1]
  for ww in ml_set:
    new_topics_index[ww] = -1
    for jj in index[ww].keys():
      relevant_docs[jj].add(ww)

  topic_assignments_in = open("%s.topic_assignments" % input_base)
  path_assignments_in = open("%s.path_assignments" % input_base)
  docs_in = open("%s.doc_id" % input_base)

  topic_assignments_out = open("%s.topic_assignments" % output_base, 'w')
  path_assignments_out = open("%s.path_assignments" % output_base, 'w')
  docs_out = open("%s.doc_id" % output_base, 'w')


  for topic_assign, path_assign, name in zip(topic_assignments_in,
                                             path_assignments_in, docs_in):
    docs_out.write(name)
    name = int(name.split()[1])
    if name in relevant_docs.keys():
      if option == 1:
        # option == 1
        # assign words to the new topics, the other words in the same doc to -1
        num_words = len(topic_assign.split()) - 1
        topic_assign_new = "%i %s" % (num_words, " ".join(["-1"] * num_words))
        topics = topic_assign_new.split()

        num_words = len(path_assign.split()) - 1
        path_assign_new = "%i %s" % (num_words, " ".join(["-1"] * num_words))
        path = path_assign_new.split()
      else:
        # option == 0
        # only assign words to the new topics, the other words keep the same
        topics = topic_assign.split()
        path = path_assign.split()

      for w in relevant_docs[name]:
        tt = new_topics_index[w]
        for win in index[w][name]:
        # int(j) + 1, because the first item each line
        # is the number of words in that document
          win_new = int(win) + 1
          topics[win_new] = str(tt)
          path[win_new] = -1
      topic_assignments_out.write("%s\n" % (" ".join(topics)))
      path_assignments_out.write("%s\n" % (" ".join(path)))

    else:
      topic_assignments_out.write(topic_assign)
      path_assignments_out.write(path_assign)

  topic_assignments_in.close()
  topic_assignments_out.close()
  path_assignments_in.close()
  path_assignments_out.close()
  docs_in.close()
  docs_out.close()

  return new_topics + 1


flags.define_string("corpus", None, "Where we find the input corpora")
flags.define_string("mapping", None, "Filename of mapping")
flags.define_string("cons_file", "", "Constraints filename")
flags.define_glob("wordnet", "wn/output.0", "contraint source")
flags.define_string("input_base", "output/nih", "Input filename")
flags.define_string("output_base", "output/nih_ned", "Output filename")
flags.define_string("resume_type", "clear", "resume type: clear or split")
flags.define_string("update_strategy", "doc", "update strategy: term or doc")
flags.define_int("doc_limit", -1, "Number of documents to process")
flags.define_int("num_topics", 0, "Current number of topics")

if __name__ == "__main__":
  flags.InitFlags()

  if re.search("doc", flags.update_strategy):
    update_strategy = 1
  elif re.search("term", flags.update_strategy):
    update_strategy = 0
  else:
    print "Wrong update strategy!"
    exit()

  # Build index if it doesn't already exist
  if os.path.exists(flags.mapping):
    index = read_pickle(flags.mapping)
  else:
    index = build_index(flags.corpus, flags.mapping)

  # Remove offending assignments
  if re.search("clear", flags.resume_type):
    # Read in constraints
    cons_set = get_constraints(flags.cons_file)
    print cons_set
    clear_assignments(flags.input_base, flags.output_base, index,
                      cons_set, update_strategy)
  elif re.search("split", flags.resume_type):
    # Read in constraints
    print flags.wordnet
    [cons_list, cons_set] = get_constraints_from_wn(flags.wordnet)
    print cons_set

    num_topics = int(flags.num_topics)
    new_num_topics = split_assignments(flags.input_base, flags.output_base,
                     index, cons_list, update_strategy, num_topics)
    print "Update number of topics: ", new_num_topics
  else:
    print "Wrong resume type!"
    exit()

  # Copy the parameter information and the document id
  if flags.input_base != flags.output_base:
    for ext in ["params"]:
      shutil.copyfile("%s.%s" % (flags.input_base, ext), "%s.%s" % \
                                (flags.output_base, ext))

    for ext in ["lhood", "param_hist"]:
      inputfile = "%s.%s" % (flags.input_base, ext)
      if os.path.exists(inputfile):
        shutil.copyfile(inputfile, "%s.%s" % (flags.output_base, ext))
