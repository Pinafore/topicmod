package lda.utils;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import utils.Utils;

/**
 * Class that calculates a list of bigrams, ordered from highest to lowest
 * log likelihood, from a list of unigrams representing the words in the
 * data set.
 */
public class BigramList
{
	private Map<String, Double> theLLMap;
	
	public BigramList(List<String> words, boolean removeStopwords)
	{	
		calculate(words);
	}
	
	public List<String> getTopBigrams(int n)
	{
		System.out.println("Started top bigrams");
		List<String> bigrams = Utils.sortByValue(theLLMap);
		System.out.println("Ended top bigrams");
		
		return bigrams.subList(0, n);
	}
	
	
	private void calculate(List<String> words)
	{
		Map<String, Integer> firstCounts = new HashMap<String, Integer>();
		Map<String, Integer> secondCounts = new HashMap<String, Integer>();
		Map<String, Integer> bigrams = new HashMap<String, Integer>();
		theLLMap = new HashMap<String, Double>();
		
		int numBigrams = words.size() - 1;
		
		for (int i = 0; i < words.size() - 1; i++)
		{
			if (i % 1000 == 0)
				System.out.println(i);
			
			String word1 = words.get(i);
			String word2 = words.get(i+1);
			
			Utils.addToMap(firstCounts, word1);
			Utils.addToMap(secondCounts, word2);
			Utils.addToMap(bigrams, word1 + " " + word2);
		}
		int cnt = 0;
		
		// now calculate the log likelihood
		for (String bigram : bigrams.keySet())
		{
			cnt++;
			if (cnt % 1000 == 0)
				System.out.println(cnt);
			
			String[] parts = bigram.split(" ");
			double o11 = bigrams.get(bigram);
			double o12 = firstCounts.get(parts[0]);
			double o21 = secondCounts.get(parts[1]);
			double o22 = numBigrams - o21 - o12 + o11;
		
			double c1Total = o11 + o21;
			double c2Total = o12 + o22;
			
			double e1 = c1Total * (o11 + o12) / (c1Total + c2Total);
			double e2 = c2Total * (o11 + o12) / (c1Total + c2Total);
			
			double ll = o11 * Math.log(o11 / e1) + o12 * Math.log(o12 / e2);
			ll *= 2;
			
			theLLMap.put(bigram, ll);
		}
	}
	


}
