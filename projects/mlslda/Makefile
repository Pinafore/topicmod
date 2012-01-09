include ../ldawn/base.mk

PYTHON_SCRIPTS=$(wildcard scripts/*.py)
MLSLDA_DEP=$(wildcard src/*.cpp src/*.h)
MLSLDA_CPP=src/mlslda.cpp

TOY_CORPUS=wn/rev_toy.wn.0 wn/rev_toy.wn.ba1 data/review_toy_corpus_english_0.index data/review_toy_corpus_german_0.index

# CODE

mlslda: $(MLSLDA_DEP) $(LDAWN_OBJ) $(OBJ_FILES)
	cpplint.py $(LINT_OPTIONS) src/mlslda.cpp $(MLSLDA_CPP)
	$(GPP) $(CFLAGS) $(INCLUDEDIRS) $(LIBDIRS) $(OBJ_FILES) $(LDAWN_OBJ) $(WORDNET_PROTO_OBJ) $(MLSLDA_CPP) src/mlslda_main.cpp -o mlslda 

test_mlslda: test/test_mlslda.cpp $(OBJ_FILES) $(MLSLDA_DEP) $(LDAWN_OBJ) $(WORDNET_PROTO_OBJ)
	cpplint.py $(LINT_OPTIONS) test/test_mlslda.cpp $(MLSLDA_DEP)
	$(GPP) $(CFLAGS) $(INCLUDEDIRS) $(LIBDIRS) $(OBJ_FILES) $(LDAWN_OBJ) $(WORDNET_PROTO_OBJ) $(MLSLDA_CPP) test/test_mlslda.cpp -o test_mlslda
	./test_mlslda

# CORPORA

$(TOY_CORPUS): scripts/toy_corpus.py $(WORDNET_PROTO_OBJ) $(WORDNET_MUNGER) $(PYTHON_SCRIPTS) $(PYTHON_PREPROCESSING)
	echo "FOUND PYTHON:$(ALT_PYTHON)"
	mkdir -p wn
	mkdir -p data
	mkdir -p vocab
	$(PYTHON_COMMAND) ../../lib/corpora/vocab.py --min_freq=0 --output=vocab/toy.voc --corpus_parts=data/review_toy_corpus_*.index
	$(PYTHON_COMMAND) scripts/toy_corpus.py
	$(PYTHON_COMMAND) ../../lib/corpora/flat_tree_writer.py --vocab=vocab/toy.voc --output=wn/toy_flat.wn

# EXPERIMENTS

wn/amzn_dict_zh.wn.0 wn/dict_de.wn.0 wn/dict_dezh.wn: vocab/amazon.voc
	$(PYTHON_COMMAND) ../../lib/corpora/multilingual_ontologies.py --output=wn/amzn_dict_zh.wn --dictionary=True --chinese=True --german=False --filter_vocab=vocab/amazon.voc
	$(PYTHON_COMMAND) ../../lib/corpora/multilingual_ontologies.py --output=wn/amzn_dict_de.wn --dictionary=True --chinese=False --german=True --filter_vocab=vocab/amazon.voc
	$(PYTHON_COMMAND) ../../lib/corpora/multilingual_ontologies.py --output=wn/amzn_dict_dezh.wn --dictionary=True --chinese=True --german=True --filter_vocab=vocab/amazon.voc

wn/movies_dict.wn.0: vocab/movies.voc ../../lib/corpora/multilingual_ontologies.py
	$(PYTHON_COMMAND) ../../lib/corpora/multilingual_ontologies.py --output=wn/movies_dict.wn --dictionary=True --chinese=False --german=True --filter_vocab=vocab/movies.voc --stem=True --id_strings=True

wn/trans.wn.0: 
	$(PYTHON_COMMAND) ../../lib/corpora/multilingual_ontologies.py --output=wn/trans.wn --translation=True --chinese=True --german=True

wn/europarl_muto.wn.0 wn/europarl_dict.wn.0 wn/europarl_flat.wn.0: vocab/europarl.voc
	$(PYTHON_COMMAND) ../../lib/corpora/multilingual_ontologies.py --output=wn/europarl_muto.wn --german=True --limit=250 --greedy_matching=True --filter_vocab=vocab/europarl.voc
	$(PYTHON_COMMAND) ../../lib/corpora/multilingual_ontologies.py --output=wn/europarl_dict.wn --dictionary=True --chinese=False --german=True --filter_vocab=vocab/europarl.voc --stem=True
	$(PYTHON_COMMAND) ../../lib/corpora/flat_tree_writer.py --vocab=vocab/europarl.voc --output=wn/europarl_flat.wn

wn/gn_bal_stem.wn.0: $(MULTILINGUAL_SCRIPTS)
	$(PYTHON_COMMAND) preprocessing/germanet_wn.py --stem=True
	$(PYTHON_COMMAND) preprocessing/germanet_wn.py 

wn/trans_wn.wn.0: $(MULTILINGUAL_SCRIPTS)
	$(PYTHON_COMMAND) ../../lib/corpora/multilingual_ontologies.py --output=wn/trans_wn.wn --dictionary=True --chinese=True --german=True --wordnet=True

multilingual_ontologies: wn/dict.wn.0 wn/trans.wn.0 wn/gn_bal.wn.0

toy_mult: mlslda $(TOY_CORPUS)
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/toy_wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index,review_toy_corpus_german_0.index --output_location=output/toy_mult --num_iter=100

output/0-toy_en.lhood: mlslda $(TOY_CORPUS)
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/0-toy_en --rand=0 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/1-toy_en --rand=1 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/2-toy_en --rand=2 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/0-toy_en-lda --rand=0 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/1-toy_en-lda --rand=1 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/2-toy_en-lda --rand=2 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/0-toy_en-wn --rand=0 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/1-toy_en-wn --rand=1 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/2-toy_en-wn --rand=2 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/0-toy_en-lda-wn --rand=0 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/1-toy_en-lda-wn --rand=1 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/2-toy_en-lda-wn --rand=2 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-0-toy_en --rand=0 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-1-toy_en --rand=1 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-2-toy_en --rand=2 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-0-toy_en-lda --rand=0 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-1-toy_en-lda --rand=1 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-2-toy_en-lda --rand=2 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-0-toy_en-wn --rand=0 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-1-toy_en-wn --rand=1 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-2-toy_en-wn --rand=2 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-0-toy_en-lda-wn --rand=0 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-1-toy_en-lda-wn --rand=1 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True
	./mlslda --min_words=0 --num_topics=1 --vocab_file=vocab/toy.voc --wordnet_location=wn/rev_toy.wn.0,wn/rev_toy.wn.1 --corpus_location=data --train_section=review_toy_corpus_english_0.index --output_location=output/single-2-toy_en-lda-wn --rand=2 --alpha=1.0 --test_section=review_toy_corpus_english_0.index --num_iter=500 --regression_only=True

toy_de: mlslda $(TOY_CORPUS)
	./mlslda --min_words=0 --num_topics=5 --vocab_file=vocab/toy.voc --wordnet_location=wn/toy_flat.wn.0,wn/toy_flat.wn.1 --corpus_location=data --train_section=review_toy_corpus_german_0.index --output_location=output/toy_de

toy_graphs: output/0-toy_en.lhood
	$(PYTHON_COMMAND) scripts/merge_data_frame.py --file_patterns=output/*-toy_en*about --output_file=output/toy_en.csv

EXPER_TOPICS=25

plots:
	mkdir -p output/plots
	$(PYTHON_COMMAND) scripts/merge_data_frame.py --iteration_gap=15 --file_patterns=output/pl_*/*/*about --output_file=output/main.csv
	cat scripts/create_plots.R | R10 --no-save output/*.csv
	cp output/plots/*.png ~/public_html/temp/mlslda

all_data: wn/gn_bal.wn.0 wn/europarl_dict.wn.0 wn/movies_dict.wn.0

# TODO(jbg): Add in dependencies for other connections between languages
experiments: mlslda $(MUNGED_WORDNET)
	./scripts/run_experiments.sh 16

vocab/synthetic.voc: scripts/synthetic_data.py $(PROTO_OUT) $(WORDNET_PROTO_OUT)
	mkdir -p data/synthetic
	mkdir -p vocab
	rm -f data/synthetic/synth/*
	$(PYTHON_COMMAND) scripts/synthetic_data.py --num_langs=1 --num_docs=200 --num_topics=5  --variance=0.001 --alpha=0.1 --doc_length=15
	$(PYTHON_COMMAND) ../../lib/corpora/flat_tree_writer.py --vocab=vocab/synthetic.voc --output=wn/synthetic.wn

# Write amazon corpus (English only) in ldac format
data/lda_amazon.lda: ../../lib/corpora/ldac_format_writer.py
	$(PYTHON_COMMAND) ../../lib/corpora/ldac_format_writer.py  --doc_roots=data/amazon/amazon_english_1.index --location=data/amazon/ --num_docs=1500 --min_length=50 --vocab=vocab/en_amazon.voc --output="data/lda_amazon"

# Writes pang lee movie dataset (English only) in ldac format
data/lda_movies.lda: ../../lib/corpora/ldac_format_writer.py $(MOVIES_NUMERIC)
	$(PYTHON_COMMAND) ../../lib/corpora/ldac_format_writer.py  --doc_roots=data/movies/movies_english_*.index --location=data/movies/ --min_length=50 --vocab=vocab/movies.voc --output="data/lda_movies" --num_docs=5000

output/hellinger.pdf: scripts/hellinger.R
	cat scripts/hellinger.R | R --no-save output/hellinger.pdf output/europarl/*/*True-False.hellinger