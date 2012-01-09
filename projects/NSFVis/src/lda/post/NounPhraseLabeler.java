package lda.post;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import utils.Constants;
import utils.Utils;

/**
 * Class that gets the noun phrase labels for a set of topics or
 * topic/year pairs. A list of candidate noun phrases must already
 * exist in a file, since generating the noun phrases is much too 
 * time intensive to do on the fly. 
 */
public class NounPhraseLabeler
{
	public static String[] getLabels(RawTopicInfo rawInfo, String filename)
	{
		String[] output = new String[rawInfo.numTopics];
		List<String> nounPhrases = getNounPhrases(filename);
		for (int i = 0; i < rawInfo.numTopics; i++)
		{
			List<String> bigrams = label(rawInfo, nounPhrases, i, -1,
					Constants.NUM_NOUN_PHRASES);
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

	public static String[][] getYearLabels(RawTopicInfo rawInfo,
			String filename, int numYears)
	{
		String[][] output = new String[numYears][rawInfo.numTopics];
		List<String> nounPhrases = getNounPhrases(filename);
		for (int year = 0; year < numYears; year++)
		{
			for (int i = 0; i < rawInfo.numTopics; i++)
			{
				List<String> bigrams = label(rawInfo, nounPhrases, i, year,
						Constants.NUM_YEAR_NOUN_PHRASES);
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

	private static List<String> getNounPhrases(String filename)
	{
		try
		{
			return Utils.readAll(filename);
		} catch (Exception e)
		{
			e.printStackTrace();
			return new ArrayList<String>();
		}
	}

	private static List<String> label(RawTopicInfo rawInfo,
			List<String> nounPhrases, int topic, int year, int numWords)
	{

		Map<String, Double> map = new HashMap<String, Double>();
		for (String nounPhrase : nounPhrases)
		{
			Map<String, Integer> wordMap = null;
			int wordCount = 0;
			if (year < 0)
			{
				wordMap = rawInfo.topicWordCounts[topic];
				wordCount = rawInfo.topicTotalWords[topic];
			} else
			{
				wordMap = rawInfo.yearTopicWordCounts[year][topic];
				wordCount = rawInfo.yearTopicTotalWords[year][topic];
			}

			String[] words = nounPhrase.split(" ");
			boolean hasWords = true;
			for (String word : words)
			{
				hasWords = hasWords && (wordMap.containsKey(word));
			}

			if (hasWords)
			{
				double total = 0;

				for (String word : words)
				{
					double value = wordMap.get(word) * 1.0 / wordCount;
					value *= -Math
							.log((rawInfo.wordCounts.get(word) * 1.0 / rawInfo.totalWords));
					total += value;
				}

				total = total / words.length;
				map.put(nounPhrase, total);
			}
		}

		List<String> results = Utils.sortByValue(map);
		if (results.size() > numWords)
			return results.subList(0, numWords);
		else
			return results;
	}

}
