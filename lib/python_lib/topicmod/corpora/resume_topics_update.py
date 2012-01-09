import re
import os.path
from proto.corpus_pb2 import *
from proto.wordnet_file_pb2 import *
from topicmod.util import flags
from topicmod.util.sets import read_pickle, write_pickle

flags.define_int("option", 0, \
   "change the whole documents or just the topics of just the word")
flags.define_string("ldawnoutput", "output/nsf", "ldawn output directory")
flags.define_string("maps", "output/nsf", "mapping files directory")
flags.define_string("wordnet", "wn/output.0", "contraint source")
flags.define_string("assignment_path", None, "Where the assignments live")

def checkSame(cons, old_cons):
    if len(cons) != len(old_cons):
        return False
    for key in cons:
        if key not in old_cons:
            return False
    return True
  
  
def getMappingDicts_reGen(corpusdir, mapsdir, cons):
    # check the old constraint.dict exists or not
    cons_file = corpusdir + "/constraint.set"
    if (not os.path.exists(cons_file)):
        # Regenerate
        (word_wid_dic, wid_did_dic, did_doc_dic) = \
            getNewMappingDicts(corpusdir, mapsdir)
    else:
        # check whether the old constraint is the same as consdict
        old_cons = read_pickle(cons_file)
        if checkSame(cons, old_cons):
            # check the mapping dicts exist or not
            word_wid = mapsdir + "/word_wid.dict"
            wid_did = mapsdir + "/wid_did.dict"
            did_doc = mapsdir + "/did_doc.dict"
      
            if (os.path.exists(word_wid) and os.path.exists(wid_did) \
                                         and os.path.exists(did_doc)):
                word_wid_dic = read_pickle(word_wid)
                wid_did_dic = read_pickle(wid_did)
                did_doc_dic = read_pickle(did_doc)
            else:
                (word_wid_dic, wid_did_dic, did_doc_dic) = \
                    getNewMappingDicts(corpusdir, mapsdir)
        else:
            (word_wid_dic, wid_did_dic, did_doc_dic) = \
                getNewMappingDicts(corpusdir, mapsdir)
    write_pickle(cons, cons_file)
    return (word_wid_dic, wid_did_dic, did_doc_dic)
  
  
def getMappingDicts(corpusdir, mapsdir):
    # check the mapping dicts exist or not
    word_wid = mapsdir + "/word_wid.dict"
    wid_did = mapsdir + "/wid_did.dict"
    did_doc = mapsdir + "/did_doc.dict"
  
    if (os.path.exists(word_wid) and os.path.exists(wid_did) \
                                 and os.path.exists(did_doc)):
        word_wid_dic = read_pickle(word_wid)
        wid_did_dic = read_pickle(wid_did)
        did_doc_dic = read_pickle(did_doc)
    else:
        (word_wid_dic, wid_did_dic, did_doc_dic) = \
            getNewMappingDicts(corpusdir, mapsdir)
      
    return (word_wid_dic, wid_did_dic, did_doc_dic)
  
  
def getNewMappingDicts(corpusdir, mapsdir):

    # Mapping documents to word
    corpusLocation = corpusdir + "/model_topic_assign/doc_voc.index"
  
    inputfile = open(corpusLocation, 'rb')
    protocorpus = Corpus()
    protocorpus.ParseFromString(inputfile.read())
  
    voc_tokens = protocorpus.tokens
  
    word_wid_dic = dict()
    wid_word_dic = dict()
    wid_did_dic = dict()
    did_doc_dic = dict()
  
    for i in range(0, len(voc_tokens)):
        terms = voc_tokens[i].terms
        for j in range(0, len(terms)):
            w = terms[j].original
            id = terms[j].id
            word_wid_dic[str(w)] = id
            wid_word_dic[id] = str(w)
            wid_did_dic[id] = dict()
      
    docnames = protocorpus.doc_filenames
  
    for i in range(0, len(docnames)):
        docname = docnames[i]
        doclocation = corpusdir + "/model_topic_assign/" + docname
        docfile = open(doclocation, 'rb')
        doc = Document()
        doc.ParseFromString(docfile.read())
    
        did = i
        did_doc_dic[did] = docname
    
        sents = doc.sentences
        word_index = 0
    
        for j in range(0, len(sents)):
            words = sents[j].words
            for k in range(0, len(words)):
                w = words[k].token
                #wid_did_dic[w].add(did)
                if did not in wid_did_dic[w]:
                    wid_did_dic[w][did] = set()
                wid_did_dic[w][did].add(word_index)
                word_index += 1
        
    # Save mapping dicts
    word_wid = mapsdir + "/word_wid.dict"
    wid_did = mapsdir + "/wid_did.dict"
    did_doc = mapsdir + "/did_doc.dict"
  
    write_pickle(word_wid_dic, word_wid)
    write_pickle(wid_did_dic, wid_did)
    write_pickle(did_doc_dic, did_doc)
  
    return (word_wid_dic, wid_did_dic, did_doc_dic)
  
def getConstraints(wnLocation):
    wnfile = open(wnLocation, 'rb')
    wn = WordNetFile()
    wn.ParseFromString(wnfile.read())
  
    synsets = wn.synsets
    cons_list = []
    cons_set = set()
    for i in range(0, len(synsets)):
        name = synsets[i].key
        if "constraint" in name:
            tmp = []
            words = synsets[i].words
            for j in range(0, len(words)):
                w = words[j].term_str
                tmp.append(str(w))
                cons_set.add(str(w))
            print wnLocation, "CONSTRAINT FOUND:", tmp
            cons_list.append(tmp)
    return cons_set, cons_list
  
  
def getNewAddedCons(corpusdir, cons_set, cons_list):
    # check the old constraint.list exists or not
    cons_file = corpusdir + "/constraint.set"
    if (not os.path.exists(cons_file)):
        cons_added_set = cons_set
    else:
        cons_old_set = read_pickle(cons_file)
        cons_added_set = cons_set.difference(cons_old_set)
    # save the new cons set to file
    write_pickle(cons_set, cons_file)
    cons_file = corpusdir + "/constraint.list"
    write_pickle(cons_list, cons_file)
    return cons_added_set
  
  
def getConstraintsDict(cons, word_wid_dic):
    consdict = dict()
    for w in cons:
        id = word_wid_dic[w]
        consdict[id] = w
    return consdict
  
  
def updateTopics(topicLocation, wid_did_dic, consdict, option):
    topicfile = open(topicLocation, 'rb')
  
    topicupdate = []
    for line in topicfile:
        line = line.strip()
        words = line.split(" ")
        topicupdate.append(words)
    
    topicfile.close()
  
    # assign topic of constraint words to be "-1"
  
    for key in consdict.keys():
        docs = wid_did_dic[key]
        for did in docs.keys():
            int_did = int(did)
            # print int_did
            if option == 1:
                num = len(topicupdate[int_did])
                # not range(0, num), because the first item each line 
                # is the number of words in that document 
                for j in range(1, num):
                    topicupdate[int_did][j] = -1
            else:      
                for j in docs[did]:
                # int(j) + 1, because the first item each line 
                # is the number of words in that document 
                    int_j = int(j) + 1
                    topicupdate[int_did][int_j] = -1
          
    topicfile = open(topicLocation, 'wb')
    for i in range(0, len(topicupdate)):
        tmp = ""
        for j in range(0, len(topicupdate[i])):
            tmp += str(topicupdate[i][j]) + " "
        tmp += "\n"
        topicfile.write(tmp)
    
    topicfile.close()
  
  
if __name__ == "__main__":
    flags.InitFlags()
    corpusdir = flags.ldawnoutput
    mapsdir = flags.maps 
  
    # get mapping dicts
    (word_wid_dic, wid_did_dic, did_doc_dic) = \
                     getMappingDicts(corpusdir, mapsdir)
    
  
    # get constraints
    wnLocation = flags.wordnet
    (cons_set, cons_list) = getConstraints(wnLocation)
    
    # get constraints dict
    # 1. only update the new added constraints
    cons_added_set = getNewAddedCons(corpusdir, cons_set, cons_list)
    print cons_added_set
    consdict = getConstraintsDict(cons_added_set, word_wid_dic)
    # 2. update all the constrains
    # consdict = getConstraintsDict(cons, word_wid_dic)
  
    # update topic assignment: assign topic of constraint words to be "-1"
    topicLocation = corpusdir + "/model.topic_assignments"
    # option == 1: set all the topics in that document as "-1"
    # option == 0: set only the constraint words in that document as "-1"
    option = flags.option
    option = int(option)
    updateTopics(topicLocation, wid_did_dic, consdict, option)
