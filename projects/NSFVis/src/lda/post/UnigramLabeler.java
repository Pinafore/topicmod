package lda.post;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import utils.Constants;
import utils.Utils;

/**
 * Class that gets the unigram labels for a set of topics or
 * topic/year pairs.
 */
public class UnigramLabeler
{
	
	private static List<String> label(RawTopicInfo rawInfo, int topic, int year, int numWords)
	{
		Map<String, Double> map = new HashMap<String, Double>();
		for (String word : rawInfo.wordCounts.keySet())
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
			
			boolean hasWords = wordMap.containsKey(word);

			if (hasWords)
			{
				
				double value1 = wordMap.get(word) * 1.0 / wordCount;
				value1 *= -Math.log((rawInfo.wordCounts.get(word) * 1.0 /
					rawInfo.totalWords));
				
				map.put(word, value1);
			}
		}
		
		List<String> results = Utils.sortByValue(map);
		if (results.size() > numWords)
			return results.subList(0, numWords);
		else
			return results;
	}
	
	public static String[] getLabels(RawTopicInfo rawInfo, int numWords)
	{
		String[] labels = new String[rawInfo.numTopics];
		for (int i = 0; i < rawInfo.numTopics; i++)
		{
			String output = "";
			List<String> topWords = label(rawInfo, i, -1, numWords);
			
			for (String word : topWords)
			{
				output += " " + word;
			}
			if (output.length() > 0)
				output = output.substring(1);
			
			labels[i] = output;
			System.out.println(i + " - " + labels[i]);
		}
		return labels;
	}
	
	public static String[][] getYearLabels(RawTopicInfo rawInfo,  int numWords, int numYears)
	{
		String[][] output = new String[numYears][rawInfo.numTopics];
		for (int year = 0; year < numYears; year++)
		{
			for (int i = 0; i < rawInfo.numTopics; i++)
			{
				String outputStr = "";
				List<String> topWords = label(rawInfo, i, year, numWords);
				
				for (String word : topWords)
				{
					outputStr += " " + word;
				}
				if (outputStr.length() > 0)
					outputStr = outputStr.substring(1);				
				output[year][i] = outputStr;
			}
		}
		return output;
	}	
	
	public static String[][] getTopicWords(RawTopicInfo rawInfo, int numWords)
	{
		String[][] words = new String[rawInfo.numTopics][];
		for (int i = 0; i < rawInfo.numTopics; i++)
		{
			List<String> topWords = label(rawInfo, i, -1, numWords);
			words[i] = topWords.toArray(new String[0]);
		}
		return words;
	}
	
	
}
