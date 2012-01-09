package lda.post;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import utils.Constants;
import utils.DataSet;
import utils.LabelType;
import utils.DocumentInfo;
import utils.TopicInfo;
import utils.Utils;

public class TopicInfoGetter
{
	public static TopicInfo getTopicInfo (
			DataSet dataSet,
			RawTopicInfo rawInfo,
			List<DocumentInfo> propInfo,
			List<Integer> years,
			LabelType labelType)
	{
		TopicInfo info = new TopicInfo();
		info.setNumTopics(rawInfo.numTopics);
		info.setYears(years);
		
		updateProposalInfo(rawInfo, propInfo);
		info.setProposalInfo(propInfo);

		double[][] yearProps = getYearProps(rawInfo, propInfo, years.size());
		info.setYearProps(yearProps);
		
		int[][] topicOrder = getTopicOrder(yearProps);
		info.setTopicOrder(topicOrder);
		
		String[][] topicWords = UnigramLabeler.getTopicWords(rawInfo, Constants.NUM_TOPIC_WORDS);
		info.setTopicWords(topicWords);
		
		info.setUnigramLabels(UnigramLabeler.getLabels(rawInfo, Constants.NUM_UNIGRAMS));
		info.setBigramLabels(BigramLabeler.getLabels(rawInfo));
		info.setNounPhraseLabels(NounPhraseLabeler.getLabels(rawInfo, dataSet.nounPhraseFilename));
		
		info.setYearUnigramLabels(UnigramLabeler.getYearLabels(rawInfo, Constants.NUM_YEAR_UNIGRAMS, dataSet.years.size()));
		info.setYearBigramLabels(BigramLabeler.getYearLabels(rawInfo, dataSet.years.size()));
		info.setYearNounPhraseLabels(NounPhraseLabeler.getYearLabels(rawInfo, dataSet.nounPhraseFilename, dataSet.years.size()));
		
		return info;
	}
	
	private static void updateProposalInfo(RawTopicInfo rawInfo,
			List<DocumentInfo> propInfoList)
	{
		for (int i = 0; i < rawInfo.docIds.length; i++)
		{
			String id = rawInfo.docIds[i];
			DocumentInfo propInfo = getByID(propInfoList, id);
			
			if (propInfo == null)
			{
				System.out.println("Proposal info was null for ID: " + id);
			}
			else
			{
				propInfo.topicProps = new double[rawInfo.numTopics];
				for (int j = 0; j < rawInfo.numTopics; j++)
				{
					propInfo.topicProps[j] = rawInfo.docTopicWordCounts[i][j] * 1.0 /
						rawInfo.docWordCounts[i];
				}
			}
		}
	}
	
	private static double[] getRelativeDocumentsPerYear(List<DocumentInfo> propInfoList,
			int numYears)
	{
		double[] count = new double[numYears];
		double maxCount = 0;
		
		for (DocumentInfo propInfo : propInfoList)
		{
			int year = propInfo.year;
			count[year]++;
			maxCount = Math.max(maxCount, count[year]);
		}
		
		for (int i = 0; i < numYears; i++)
		{
			count[i] = count[i] / maxCount;
		}
		return count;
	}
	
	private static double[][] getYearProps(RawTopicInfo rawInfo,
			List<DocumentInfo> propInfoList,
			int numYears)
	{
		double[][] yearProps = new double[numYears][rawInfo.numTopics];
		double[] yearWordCounts = new double[numYears];
		
		for (int i = 0; i < rawInfo.docIds.length; i++)
		{
			String id = rawInfo.docIds[i];
			DocumentInfo propInfo = getByID(propInfoList, id);
			if (propInfo != null)
			{
			
				int year = propInfo.year;
				
				for (int j = 0; j < rawInfo.numTopics; j++)
				{
					yearProps[year][j] += rawInfo.docTopicWordCounts[i][j];
				}
				yearWordCounts[year] += rawInfo.docWordCounts[i];
			}
		}		
		
		for (int i = 0; i < numYears; i++)
			for (int j = 0; j < rawInfo.numTopics; j++)
			{
				yearProps[i][j] = yearProps[i][j] * 1.0 / yearWordCounts[i];
			}
		
		// finally scale them
		double[] relDocuments = getRelativeDocumentsPerYear(propInfoList, numYears);
		for (int i = 0; i < numYears; i++)
			for (int j = 0; j < rawInfo.numTopics; j++)
			{
				yearProps[i][j] = yearProps[i][j] * relDocuments[i];
			}
		
		return yearProps;
		                                     
	}
	
	private static int[][] getTopicOrder(double[][] yearProps)
	{
		int[][] order = new int[yearProps.length][];
		for (int i = 0; i < yearProps.length; i++)
		{
			Map<Integer, Double> map = new HashMap<Integer, Double>();
			for (int j = 0; j < yearProps[i].length; j++)
			{
				map.put(j, yearProps[i][j]);
			}
			List<Integer> sortedList = Utils.sortByValue(map);
			
			order[i] = listToArray(sortedList);
		}
		return order;
	}
	
	private static DocumentInfo getByID(List<DocumentInfo> propInfoList,
			String id)
	{
		for (DocumentInfo info : propInfoList)
		{
			if (info.id.equals(id))
				return info;
		}
		return null;
	}
	
	private static int[] listToArray(List<Integer> list)
	{
		int[] array = new int[list.size()];
		for (int i = 0; i < list.size(); i++)
		{
			array[i] = list.get(i);
		}
		return array;
	}

}
