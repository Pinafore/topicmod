Prepare:
1. Please read and follow https://code.google.com/p/topicmod/wiki carefully
2. make sure you installed all the package on the wiki
3. compile: 
   cd topicmmod/projects/ldawn
   make ldawn

Example of using iteractive topic modeling:

1. generate protofiles for 20_news group data:
   cd ../../data
   sh unzip_data.sh
   cd ../projects/ldawn
   make ../../data/20_news_date/numeric/20_news_english_0.index

2. generate vocab:
   make vocab/20_news.voc

3. firstly generate tree structure with empty constraints (the initial topic modeling is running without any constraints):
   mkdir -p wn
   PYTHONPATH="$PYTHONPATH:../../lib/python_lib/:." python ../../lib/python_lib/topicmod/corpora/ontology_writer.py --vocab=vocab/20_news.voc --constraints=constraints/empty.cons --write_wordnet=False --write_constraints=True --wnname=wn/20_news_empty.wn

4. running ldawn for some iterations as a start point for ITM:
   mkdir -p output
   mkdir -p output/original
   mkdir -p output/original/model_topic_assign
   mkdir -p output/original/model_topic_assign/assign
   iter=100
   ./ldawn --train_section=`rump_comma.py ../../data/20_news_date/numeric/20_news_*.index` --corpus_location=../../data/20_news_date/numeric --vocab=vocab/20_news.voc --wordnet_location=`comma.py wn/20_news_empty.wn.*` --output_location=output/original/model --num_topics=20 --rand=0 --num_iter=$iter --global_sample_reps=0 --save_delay=10 --wn_hyperparam_file=hyperparameters/wn_hyperparams --num_topic_terms=15 --verbose_assignments=True --min_words=10

5. interactive process: Assume we resample step=50 itertaions each time:
(5.1) showing users the top words for each topic in the previous result, and get constraints from users: 20_news_example.cons
(5.2) Generate new treestructure:
      PYTHONPATH="$PYTHONPATH:../../lib/python_lib/:." python ../../lib/python_lib/topicmod/corpora/ontology_writer.py --vocab=vocab/20_news.voc --constraints=constraints/20_news_example.cons --write_wordnet=False --write_constraints=True --wnname=wn/20_news.wn
(5.3) clear topic assignments for constraint words
      mkdir -p maps
      mkdir -p output/cons1
      PYTHONPATH="$PYTHONPATH:../../lib/python_lib/:." python scripts/resume_topics.py --mapping=maps/20_news.map --wordnet="wn/20_news.wn.*" --cons_file=constraints/20_news_example.cons --corpus=output/original/model_topic_assign/ --input_base=output/original/model --output_base=output/cons1/model --resume_type=clear --update_strategy=doc --num_topics=20
(5.4) resampling for 50 iterations
      iter=$iter+20
      ./ldawn --train_section=`rump_comma.py ../../data/20_news_date/numeric/20_news_*.index` --corpus_location=../../data/20_news_date/numeric --vocab=vocab/20_news.voc --wordnet_location=`comma.py wn/20_news.wn.*` --output_location=output/cons1/model --num_topics=20 --rand=0 --num_iter=$iter --global_sample_reps=0 --save_delay=10 --wn_hyperparam_file=hyperparameters/wn_hyperparams --num_topic_terms=15 --min_words=10 --resume=True
(5.5) repeat (5.1 ~ 5.5) till the users are satisfied.
