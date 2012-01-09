
include ../base.mk

lda: src/lda_sampler.cpp $(OBJ_FILES)
	cpplint.py $(LINT_OPTIONS) src/lda_sampler.cpp
	$(GPP) $(CFLAGS) $(INCLUDEDIRS) $(LIBDIRS) $(OBJ_FILES) $(CORPORA_CPP) src/lda_sampler.cpp -o lda


run_test: vocab/semcor.voc lda
	./lda --corpus_location=../../data/semcor-3.0/numeric --vocab=vocab/semcor.voc semcor_english_1.index semcor_english_2.index