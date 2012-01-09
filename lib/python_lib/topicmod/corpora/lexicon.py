#!/usr/bin/python

import util.flags as flags
import getopt
import os
import re
import string
import xml.dom.minidom as minidom

flags.define_int("min_document_size", 1, "Minimum size of documents to output.")

class Lexicon:
    def __init__(self):
        self.int_to_lex = {}
        self.lex_to_int = {}
        self.num_lex = 0
	self.counts = {}
	self.sorted_words = None

    def Seen(self, lex):
        """Return a lexed term's index."""
        if not self.lex_to_int.has_key(lex):
            self.lex_to_int[lex] = self.num_lex
            self.int_to_lex[self.num_lex] = lex
            self.num_lex += 1
	retval = self.lex_to_int[lex]
	self.counts[retval] = self.counts.get(retval, 0) + 1
        return retval

    def Count(self, lex):
	return self.counts[lex]

    def __len__(self):
        return self.num_lex

    def Rank(self, lex):
	if self.sorted_words is None:
	    self.sorted_words = {}
            sort_queue = self.counts.keys()
            sort_queue.sort(lambda x, y: cmp(self.counts[x], self.counts[y]))
            for ii in range(0, len(sort_queue)):
                self.sorted_words[sort_queue[ii]] = float(ii) / len(sort_queue)
                if self.sorted_words[sort_queue[ii]] >= flags.max_word_frequency_rank:
		    print self.int_to_lex[sort_queue[ii]], self.sorted_words[sort_queue[ii]], self.counts[sort_queue[ii]]
        return self.sorted_words[lex]

    def Write(self, fhandle):
        for ii in range(0, self.num_lex):
            fhandle.write(self.int_to_lex[ii].encode("utf-8") + "\n")

# Like lexicon except that it writes new words as they create them.
class StreamingLexicon(Lexicon):
    def __init__(self, fhandle):
        Lexicon.__init__(self)
        self.filehandle_ = fhandle

    def Seen(self, lex):
        """Return a term's index, writing new indices to self.filehandle_."""
        if not self.lex_to_int.has_key(lex):
            self.filehandle_.write(lex.encode("utf-8") + "\n")
        return Lexicon.Seen(self, lex)    

class CountedSet:
    def __init__(self):
        self.counts_ = {}

    def Clear(self):
        self.counts_ = {}
        
    def Add(self, v, c = 1):
        self.counts_[v] = self.counts_.get(v, 0) + c

    def AddAll(self, vv):
        for v in vv:
            self.Add(v)

    def __len__(self):
        return len(self.counts_)

    def __in__(self, x):
        return self.counts_.has_key(x)

    def __getitem__(self, x):
        return self.counts_[x]

    def Incorporate(self, other):
        for ww, cc in other:
            self.counts.Add(ww, cc)

    def __iter__(self):
        for ww, cc in self.counts_.items():
            yield ww, cc

class Document:
    def __init__(self, lexicon):
        self.lexicon = lexicon
        self.lexed = []
        self.words = []
        self.counts = CountedSet()

    def __len__(self):
        return len(self.lexed)

    def Load(self, line):
        self.counts.Clear()
        line_parts = line.strip().split(' ')
        num_words = int(line_parts[0])
        line_parts = line_parts[1:]
        assert(len(line_parts) == num_words)
        for pp in line_parts:
            ww, cc = pp.split(':')
            self.counts.Add(int(ww), int(cc))

    def IncorporateDocument(self, other_document):
        self.counts.Incorporate(other_document.counts)

    def Append(self, s):
        """Add a lexed word to the seen list."""
        self.words.append(s)
        new_lexed = self.lexicon.Seen(s)
        self.lexed.append(new_lexed)
        self.counts.Add(new_lexed)

    def Concatenate(self, s):
        """Add lexed words (as a list) to the seen list."""
        self.words += s
        new_lexed = map(lambda x: self.lexicon.Seen(x), s)
        self.lexed += new_lexed
        self.counts.AddAll(new_lexed)
            
    def Write(self, fhandle, postfix = ""):
        fhandle.write(str(len(self.counts)) + " ")
        for k,v in self.counts:
            fhandle.write(str(k) + ":" + str(v) + " ")
	fhandle.write(postfix)
        fhandle.write("\n")

#     def WriteFeatures(self, fhandle, prefix = ""):
#         self.counts = {}
#         for lex in self.lexed:
#             self.counts[lex] = self.counts.get(lex, 0) + 1

#         feature = ["0"] * len(self.lexicon)
#         for k,v in self.counts.items():
#             feature[k] = str(v)
#         fhandle.write(prefix + " " + " ".join(feature) + "\n")

class Collection:
    def __init__(self):
        self.lexicon_ = Lexicon()
        self.documents_ = []
        self.toplevel_name_ = "Collection"

    def __len__(self):
        return len(self.documents_)

    def LoadDocuments(self, fh):
        for ll in fh:
            self.documents_.append(self.NewDocument())
            self.documents_[-1].Load(ll)

    def AddDocument(self, doc):
        self.documents_.append(doc)

    def AddTextDocument(self, text):
        self.documents_.append(self.NewDocument())
        self.documents_[-1].Append(text)

    def Documents(self):
        return self.documents_

    def NewDocument(self):
        return Document(self.lexicon_)

    def WriteXML(self, fh):
        impl = minidom.getDOMImplementation()
        newdoc = impl.createDocument(None, self.toplevel_name_, None)
        top_element = newdoc.documentElement
        for document in self.documents_:            
            new_element = document.CreateXMLElement(newdoc)
            top_element.appendChild(new_element)
            
        newdoc.writexml(fh, addindent="\t", newl="\n")

    def ParseFromXML(self, fh):
        dom = minidom.parse(fh)
        top_element = dom.documentElement
        assert(top_element.tagName == self.toplevel_name_)
        for cc in top_element.childNodes:
            if cc.nodeType == minidom.Node.ELEMENT_NODE:
                new_document = self.ParseElementNode(cc)
                self.documents_.append(new_document)
        dom.unlink()

    def RemoveEmptyDocuments(self):
        self.documents_ = [document for document in self.documents_ if len(document.counts) >= flags.min_document_size] 

    def WriteLexicon(self, fh):
        self.lexicon_.Write(fh)

    def WriteTitles(self, fh):
        for document in self.documents_:
            fh.write(document.title_.encode("utf-8") + "\n")

    def Write(self, fh):
        for document in self.documents_:
            document.Write(fh)
