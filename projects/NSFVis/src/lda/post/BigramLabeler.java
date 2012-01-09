package lda.post;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import utils.Constants;
import utils.Utils;

/**
 * Class that gets the bigram labels for a set of topics or
 * topic/year pairs. 
 */
public class BigramLabeler
{
	/**
	 * Generates the bigram labels for each topic.
	 */
	public static String[] getLabels(RawTopicInfo rawInfo)
	{
		String[] output = new String[rawInfo.numTopics];
		for (int i = 0; i < rawInfo.numTopics; i++)
		{
			List<String> bigrams = label(rawInfo, i, -1, Constants.NUM_BIGRAMS);
			output[i] = "";
			for (String bigram : bigrams)
				output[i] += ", " + bigram;
			if (output[i].length() > 2)
			{
				output[i] = output[i].substring(2);
			}
		}
		return output;
	}
	
	/**
	 * Generates the bigram labels for each topic/year pair.
	 */
	public static String[][] getYearLabels(RawTopicInfo rawInfo, int numYears)
	{
		String[][] output = new String[numYears][rawInfo.numTopics];
		for (int year = 0; year < numYears; year++)
		{
			for (int i = 0; i < rawInfo.numTopics; i++)
			{
				List<String> bigrams = label(rawInfo, i, year, Constants.NUM_YEAR_BIGRAMS);
				output[year][i] = "";
				for (String bigram : bigrams)
					output[year][i] += ", " + bigram;
				if (output[year][i].length() > 2)
				{
					output[year][i] = output[year][i].substring(2);
				}
			}
		}
		return output;
	}	

	/**
	 * Forms a set of top bigrams, starting with the most relevant, for a given
	 * topic or topic/year pair.
	 */
	private static List<String> label(RawTopicInfo rawInfo, int topic, int year, int numWords)
	{
		
		Map<String, Double> map = new HashMap<String, Double>();
		for (String bigram : rawInfo.bigrams)
		{
			Map<String, Integer> wordMap = null;
			int wordCount = 0;
			if (year < 0)
			{
				wordMap = rawInfo.topicWordCounts[topic];
				wordCount = rawInfo.topicTotalWords[topic];
			}
			else
			{
				wordMap = rawInfo.yearTopicWordCounts[year][topic];
				wordCount = rawInfo.yearTopicTotalWords[year][topic];
			}
			
			
			String[] words = bigram.split(" ");
			boolean hasWords = wordMap.containsKey(words[0]);	
			hasWords = hasWords && (wordMap.containsKey(words[1]));
			
			if (hasWords)
			{

				double value1 = wordMap.get(words[0]) * 1.0 / wordCount;
				value1 *= -Math.log((rawInfo.wordCounts.get(words[0]) * 1.0 /
					rawInfo.totalWords));
				
				double value2 = wordMap.get(words[1]) * 1.0 / wordCount;
				value2 *= -Math.log((rawInfo.wordCounts.get(words[1]) * 1.0 /
						rawInfo.totalWords));
				
				map.put(bigram, value1 + value2);
			}
		}
		
		List<String> results = Utils.sortByValue(map);
		if (results.size() > numWords)
			return results.subList(0, numWords);
		else
			return results;
	}
	
	
}
