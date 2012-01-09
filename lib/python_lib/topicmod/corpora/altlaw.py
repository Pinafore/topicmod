#!/usr/bin/python

__author__ = "Sean Gerrish (sgerrish@cs.princeton.edu)"
__copyright__ = "GNU Public License"

import datetime
import lexicon
import os
import re
import sys
import time

from nlp import tokenizer
from util.pdf_wrapper import PdfOpen, Clean
from xml.parsers import expat

YEAR_RE = re.compile("(1[89]\d\d|2[01]\d\d)")

class AltLawParser(lexicon.Document):
    def __init__(self, lex):
        self.parser_ = expat.ParserCreate()
        self.parser_.StartElementHandler = self.StartElementHandler
        self.parser_.EndElementHandler = self.EndElementHandler
        self.parser_.CharacterDataHandler = self.CharacterDataHandler

        self.elements_ = {}

        self.body = ""
        self.date = datetime.datetime(1, 1, 1)
        self.id = ""
        self.title = ""
        self.case_number = ""
        self.court = ""
        self.docket = ""

        lexicon.Document.__init__(self, lex)

    def Parse(self, pdf_filename, meta_pdf_filename=None):

        try:
            pdf_file = PdfOpen(pdf_filename)
        except RuntimeError:
            print >>sys.stderr, "Error parsing pdf file. Skipping."
            raise AssertionError
        self.body = pdf_file.read()
        pdf_file.close()

        if meta_pdf_filename is None:
            meta_pdf_filename = pdf_filename + ".xml"
        self.meta_pdf_filename_ = meta_pdf_filename
        if not os.path.exists(meta_pdf_filename):
            print >>sys.stserr, (
                "No document metadata found (looked in %s)." % meta_pdf_filename)
            return
        meta_pdf_file = open(meta_pdf_filename)
        self.meta_text_ = "".join(meta_pdf_file.readlines())
        self.parser_.Parse(self.meta_text_)
        meta_pdf_file.close()

    def ParseDate(self, data):
        data = data.encode("ascii", "ignore")
        try:
            self.date = datetime.datetime(
                *(time.strptime(data, "%Y/%m/%d")[0:6]))
            return
        except ValueError:
            pass
        try:
            self.date = datetime.datetime(
                *(time.strptime(data, "%m/%d/%Y")[0:6]))
            return
        except:
            pass
        try:
            date_parts = data.split("/")
            if len(date_parts[2]) == 2:
                if len(date_parts[2]) < 10:
                    date_parts[2] = "20" + date_parts[2]
                elif date_parts[2] > 10:
                    date_parts[2] = "19" + date_parts[2]
            old_data = data
            data = "/".join(date_parts)
            self.date = datetime.datetime(*(time.strptime(data, "%m/%d/%Y")[0:6]))
            print >>sys.stderr, (
                "Warning: inferring possibly ambiguous date string from %s" % old_data)
            return
        except:
            pass
        try:
            match_obj = YEAR_RE.search(data)
            if match_obj:
                # Don't bother with trying to infer
                # anything more than the year.
                self.date = datetime.datetime(int(match_obj.groups()[0]), 1, 1)
                print >>sys.stderr, (
                    "Warning: ignoring month and day for document %s"
                    % self.meta_pdf_filename_)
                return
        except IOError:
            pass
        print >>sys.stderr, (
            "Failed to parse date string '%s'" % data)

    def CharacterDataHandler(self, data):
        if "case" not in self.elements_:
            return

        if "Title" in self.elements_:
            self.title = data
        if "Court" in self.elements_:
            self.court = data
        if "CaseNum" in self.elements_:
            self.case_number = data
        if "Docket" in self.elements_:
            self.docket = data

        # Handle the date.
        if "Released" in self.elements_:
            self.ParseDate(data)
        elif "Date" in self.elements_:
            self.ParseDate(data)
        elif "DateTimeSize" in self.elements_:
            self.ParseDate(data)

    def StartElementHandler(self, name, attrs):
        if name not in self.elements_:
            self.elements_[name] = []
        self.elements_[name].append(attrs)
        
    def EndElementHandler(self, name):
        assert name in self.elements_
        assert len(self.elements_[name])
        self.elements_[name].pop()
        if not len(self.elements_[name]):
            del self.elements_[name]

    def TokenizeBody(self):
        self.Concatenate(tokenizer.Tokenize(self.body_))
            
    def Write(self, fh):
        lexicon.Document.Write(self, fh)
