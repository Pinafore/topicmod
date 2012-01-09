
This file will walk you through building a basic collapsed Gibbs sampler for
lda, generating a corpus, and running the sampler.

*** IMPORTANT ***

This program assumes that it lives in a directory called topicmod/projects/lda

Google somtimes renames the project topicmod-read-only, which causes problems.
If this happened to you, just rename the directory topicmod.

*****************

1.  First, we generate the data.  

make vocab/semcor.voc

     This requires having NLTK and the Python version of the Google protocol
     buffer library installed.  During this process it will do the following:
     read in the semcor corpus, generate tokenized output (in the numeric folder
     in the data directory), and then create a vocab file in this directory.

     If you try building the corpus and fail (perhaps because NLTK isn't
     installed, then you might get an error saying that): 

                OSError: [Errno 17] File exists:
                '../../data/semcor-3.0/numeric/semcor_english_1'

     Then simply delete the directory ../../data/semcor-3.0/numeric and try
     building the target again.

     This also requires Python 2.5.  Because it's hard to trust which is run by
     default, I create a symbolic link to the kind of Python I want to run
     (e.g. python2.4, python2.5, etc.).  This script uses that convention; this
     can be changed by modifying the PYTHON_COMMAND variable in:

         topicmod/projects/base.mk

         PYTHON_COMMAND = /usr/local/stow/python-2.5.2/bin/python
         PYTHON_COMMAND = python

         (or whatever)

     It would be nice if there were a better way of doing this automatically,
     but I don't know what that would be.

2.  Next, we'll build the sampler:

make lda

     This will try to use cpplint.py (available from Google) to check the style
     of the C++ files (it shouldn't give any errors).  cpplint.py needs to be
     available on your path.

3.  Okay, now you should be able to run a simple test:

./lda --corpus_location=../../data/semcor-3.0/numeric --vocab=vocab/semcor.voc --train_section=semcor_english_1.index,semcor_english_2.index

      This loads the semcor corpus (the first two parts, the ones tagged for all
      parts of speech).  The extra command line arguments that aren't prefixed
      are the corpus names.

      This will generate output in the directory "output" with filenames starting with "model".  

4.  What do these output files look like?  There are files that (a) save the state (b) describe the topics and (c) track progress.

    a) Saving the state

       There are two files that specify the state.  Only one of them is
       important for LDA; both are important for LDA and LDAWN (in the ldawn
       project folder).

       <model>.topic_assignments

       Has one line per document.  Each line has one number per word, separated
       by spaces.  Each number represents the current state assignment of the
       word.  The order of the word is specified by the original document (the
       order is specified by the linear order of the protocol buffer tokens that
       generated the document - note that words excluded by the corpus reader
       are not represented in this state file).  Valid numbers are from 0 to K -
       1, where K is the number of topics.  Another valid value is -1, which
       means that the current topic is unassigned.  In the next sampling step,
       it will be given a topic.

       <model>.path_assignments

       Again has one line per document.  Each line has one number per word.
       Each number represents the current path assignment of a word (when a tree
       is used as a prior for topics).  -1 represents not using a tree to
       generate the word.  Valid values are from 0 to the number of valid paths
       -1 (this depends on the topology of the tree prior).

       
