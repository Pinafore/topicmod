include ../base.mk

LDAWN_DEP=$(wildcard $(LDAWN_PATH)/src/ldawn.h $(LDAWN_PATH)/src/wordnet.h $(LDAWN_PATH)/src/*.cpp)
LDAWN_CPP=$(LDAWN_PATH)/src/wordnet.cpp $(LDAWN_PATH)/src/ldawn.cpp
LDAWN_OBJ=$(notdir $(LDAWN_CPP:.cpp=.o))

WORDNET_MUNGER=$(PYTHON_SOURCE)/corpora/ontology_writer.py
MUNGED_WORDNET=wn/wordnet.wn.0 wn/animal_food_toy.wn.0

$(LDAWN_OBJ): $(LDAWN_DEP) $(WORDNET_PROTO_OBJ) $(PROTO_OBJ)
	@echo $(LDAWN_DEP) $(WORDNET_PROTO_OBJ) $(PROTO_OUT)
	@echo $(LDAWN_OBJ)
	cpplint.py $(LINT_OPTIONS) $(LDAWN_DEP)
	$(GPP) $(CFLAGS) $(INCLUDEDIRS) $(LIBDIRS) -c $(LDAWN_CPP) 

# Create the protocol buffer that represents the english WN
# (by itself)
$(MUNGED_WORDNET): $(WORDNET_PROTO_OUT) $(WORDNET_MUNGER) 
	mkdir -p wn
	$(PYTHON_COMMAND) $(PYTHON_SOURCE)/corpora/ontology_writer.py

ldawn: $(LDAWN_PATH)/src/ldawn_main.cpp $(LDAWN_OBJ) $(OBJ_FILES)
	$(GPP)  $(CFLAGS) $(INCLUDEDIRS) $(WORDNET_PROTO_OBJ) $(LIBDIRS) $(OBJ_FILES) $(LDAWN_OBJ) $(LDAWN_PATH)/src/ldawn_main.cpp -o ldawn
