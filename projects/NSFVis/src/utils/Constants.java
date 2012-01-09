package utils;

/**
 * Important constants. When deploying on a new system, make sure
 * to update this file!
 *
 */
public class Constants 
{	
    /**
	 * Location of the properties file.
	 */
	public final static String PROPERTIES_FILE = "/tmp/lda/NSFVis.properties";

	/**
	 * Location of the CSV file specifying the data sets.
	 */
	public final static String DATASET_FILE = "/tmp/lda/datasets.csv";
	
	/**
	 * Name of the index file outputted by the LDA command. This will be used
	 * to retrieve the results from the protocol buffers.
	 */
	public final static String OUTPUT_INDEX = "doc_voc.index";
	
	/**
	 * Number of initial "stopword" topics to throw away.
	 */
	public final static int JUNK_TOPICS = 0;
	
	/**
	 * Number of words to display per topic in the Constraints page.
	 */
	public final static int NUM_TOPIC_WORDS = 20;
	
	/**
	 * Number of unigrams, bigrams, and noun phrases to display 
	 * per topic on the Visualization page. 
	 */
	public final static int NUM_UNIGRAMS = 6;
	public final static int NUM_BIGRAMS = 4;
	public final static int NUM_NOUN_PHRASES = 4;
	
	/**
	 * Number of unigrams, bigrams, and noun phrases to display per
	 * year on the visualization page when the "year summary" option is
	 * selected.
	 */
	public final static int NUM_YEAR_UNIGRAMS = 10;
	public final static int NUM_YEAR_BIGRAMS = 10;
	public final static int NUM_YEAR_NOUN_PHRASES = 10;
	
	/**
	 * Number of top bigrams (ordered by log-likelihood) to keep when
	 * forming the bigram list.
	 */
	public final static int TOP_BIGRAMS = 300;
	
	/**
	 * Iterations for the initial LDA
	 */
	public final static int START_ITERATIONS = 100;
	
	/**
	 * Additional LDA iterations to use when refining the topics.
	 */
	public final static int STEP_ITERATIONS = 25;
	
	/**
	 * Random seed to use in the LDA command when first starting a 
	 * session. Clicking "reset topics" will choose another random seed. 
	 * The initial random seed is always the same to enable use of the
	 * default files.
	 */
	public final static int DEFAULT_RANDOM_SEED = 10000;
	
	/**
	 * Maximum number of documents to display on the visualization page.
	 */
	public final static int DOCUMENTS_TO_DISPLAY = 15;
	
	/**
	 * Maximum number of unigrams to use when calculating the bigram list.
	 * Any additional unigrams are just cut off. This should only be necessary
	 * for a very large data set, like the full 20 Newsgroups data set. 
	 */
	public final static int MAX_UNIGRAMS = 50000;
	
}
