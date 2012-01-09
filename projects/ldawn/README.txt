If this is your first time running the code, be sure to look at the setup information here:
http://code.google.com/p/topicmod/w/list

This file will walk you through compiling the code, dumping WordNet into a form
readable by the program and generating a corpus.

1. Generate a corpus and a vocabulary file

make vocab/semcor.voc

     This creates a vocabulary file and munges the semcor dataset.  This
     requires NLTK and the Python version of the Google protocol buffer library
     to be installed.

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

2. Create a representation of WordNet that the code can read.

make wn/wordnet.wn.0

     This uses Wordnet 3.0, which is included in the repository.  Using a
     different WordNet will require modifying the code.

3. Compile the code

make ldawn

     This creates an executable called "ldawn"; this requires that boost,
     gsl, and Google's protocol buffers are installed.  This will try to use
     cpplint.py (available from Google) to check the style of the C++ files (it
     shouldn't give any errors).  cpplint.py needs to be available on your path.

4. Make a directory to hold output from running the model

   mkdir output

5. Run a simple test

   ./ldawn --num_iterations=10 --save_delay=5

6. Look at the output in the output directory

   a. output/model.lhood - Likelihood per iteration (one line per iteration)
      * First column: iteration
      * Second column: training lhood
      * Third column: testing lhood
      * Fourth column: disambiguation accuracy
      * Fifth column: time (in seconds) of the iteration
   b. output/model.topic_assignments - Topic assignment (one line per doc)
   c. output/model.path_assignemnts - Path assignments (one line per doc)
   d. output/model.synsets - Synset disambiguation, one blank line per doc
      * A plus at the start of the line means that the assigned synset was correct
      * A minus means the assigned synset was wrong
      * For incorrect assignments the correct synset is followed by the incorrect one
      * The ids shown in this file are the ones specified in the "key" field of
        the wordnet protocol buffer
   e. output/model.*.dot - Plots of learned topic walks (in Graphviz format, if
   you specified debug synsets)
   f. output/model.doc_totals - Empirical topic distribution (one line per doc)
   g. output/model.topics - Learned topics so far, marginalized over the walks

========================

NOTE: this must be in a directory called "topicmod".  If you check out annon
from Google code, it will be called "topicmod-read-only"; rename it to
"topicmod".  This is lame, I know; let me know if you have an easy fix.


make ../../data/20_news_date/numeric/20_news_0.index
make vocab/20_news.voc
make 20_news_wn 
make ldawn
mkdir output/20_news
./ldawn --corpus_location=../../data/20_news_date/numeric --num_topics=15 --vocab_file=vocab/20_news.voc --train_section=20_news_english_0.index --wordnet_location=wn/yn_20_news.wn.0,wn/yn_20_news.wn.1 --output_location=output/20_news/model --num_iterations=100 --save_delay=5
