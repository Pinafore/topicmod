package lda.post;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import lda.external.LDACaller;
import lda.utils.BigramList;
import lib_corpora_proto.ProtoCorpus.Corpus;
import lib_corpora_proto.ProtoCorpus.Document;
import lib_corpora_proto.ProtoCorpus.Document.Sentence.Word;
import utils.Constants;
import utils.DataSet;
import utils.DocumentInfo;
import utils.Utils;

/**
 * An intermediate state of topic information, after some processing
 * of the raw protocol buffer form. This class is not strictly necessary -
 * we could combine it with TopicInfo to perform only one round of
 * processing. But it's leftover from when we needed to switch between
 * Yuening's code and Mallet for doing LDA.
 *
 */
public class RawTopicInfo
{
	public int numTopics;
	public int numDocs;
	
	public List<String> bigrams;
	
	// ID for each document
	public String[] docIds;
	// Number of words in a given document that were labeled as a given
	// topic, indexed by [document][topic]
	public int[][] docTopicWordCounts;
	// Number of words in each document
	public int[] docWordCounts;
	
	// word count stuff
	public Map<String, Integer> wordCounts; // for the entire data set
	public Map<String, Integer>[] topicWordCounts;
	public Map<String, Integer>[][] yearTopicWordCounts; // indexed by 	[year][topic]
	// the total number of words labeled as a given topic
	public int[] topicTotalWords;
	// the total number of words labeled as a given topic, from documents 
	// belonging to a given year. indexed by [year][topic]
	public int[][] yearTopicTotalWords;
	// total words in the whole data set
	public int totalWords = 0;
	
	
	public RawTopicInfo(LDACaller.Result result, DataSet dataSet)
	{
		processProtobuf(result, dataSet);
	}
	
	private void processProtobuf(LDACaller.Result result, DataSet dataSet)
	{
		Corpus corpus = result.corpus;
		List<Document> documents = result.documents;
		
		numTopics = corpus.getNumTopics() - Constants.JUNK_TOPICS;
		numDocs = corpus.getDocFilenamesCount();
		docIds = new String[numDocs];
		docTopicWordCounts = new int[numDocs][numTopics];
		docWordCounts = new int[numDocs]; 
		
		int numYears = dataSet.years.size();
		
		wordCounts = new HashMap<String, Integer>();
		topicTotalWords = new int[numTopics];
		topicWordCounts = new Map[numTopics];
		
		yearTopicWordCounts = new Map[numYears][numTopics];
		yearTopicTotalWords = new int[numYears][numTopics];
		
		for (int i = 0; i < numTopics; i++)
			topicWordCounts[i] = new HashMap<String, Integer>();		
		
		for (int i = 0; i < numYears; i++)
			for (int j = 0; j < numTopics; j++)
				yearTopicWordCounts[i][j] = new HashMap<String, Integer>();
		
		List<String> unigrams = new ArrayList<String>();
		
		for (int i = 0; i < documents.size(); i++)
		{
			Document document = documents.get(i);
			docIds[i] = "" + document.getId();
			
			int year = 0;
			if (document.getDate() != null)
				year = dataSet.years.indexOf(document.getDate().getYear());
			if (year < 0 || year >= numYears)
				year = 0;
			
			for (Word wordObj : document.getSentences(0).getWordsList())
			{
				int topic = wordObj.getTopic();
				int lemma = wordObj.getLemma();
				String word = corpus.getLemmas(0).getTerms(lemma).getOriginal();

				unigrams.add(word);
				
				topic -= Constants.JUNK_TOPICS;
				
				if (topic >= 0)
				{
					totalWords++;
					topicTotalWords[topic]++;
					yearTopicTotalWords[year][topic]++;
					
					Utils.addToMap(wordCounts, word);
					Utils.addToMap(topicWordCounts[topic], word);
					Utils.addToMap(yearTopicWordCounts[year][topic], word);
					docWordCounts[i]++;
					docTopicWordCounts[i][topic]++;
				}
			}
		}
		
		BigramList bigramList = new BigramList(unigrams.subList(0, 
				Math.min(unigrams.size(), Constants.MAX_UNIGRAMS)), true);
		bigrams = bigramList.getTopBigrams(Constants.TOP_BIGRAMS);	
	}	
	
}
