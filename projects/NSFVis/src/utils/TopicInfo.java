package utils;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Summary of the topic information extracted from the protocol buffer
 * resulting from LDA.
 */
public class TopicInfo
{
	private int numTopics;
	private List<Integer> years;
	private int[][] topicOrder;
	private double[][] yearProps;
	private String[][] topicWords;
	private List<DocumentInfo> proposalInfo;
	
	private String[] unigramLabels;
	private String[] bigramLabels;
	private String[] nounPhraseLabels;
	
	private String[][] yearUnigramLabels;
	private String[][] yearBigramLabels;	
	private String[][] yearNounPhraseLabels;
	
	private final static double PROP_THRESHOLD = 0.2;

	public int getNumTopics()
	{
		return numTopics;
	}

	public void setNumTopics(int numTopics)
	{
		this.numTopics = numTopics;
	}

	public List<Integer> getYears()
	{
		return years;
	}

	public void setYears(List<Integer> years)
	{
		this.years = years;
	}

	public int[][] getTopicOrder()
	{
		return topicOrder;
	}

	public void setTopicOrder(int[][] topicOrder)
	{
		this.topicOrder = topicOrder;
	}

	public double[][] getYearProps()
	{
		return yearProps;
	}

	public void setYearProps(double[][] yearProps)
	{
		this.yearProps = yearProps;
	}


	public String[][] getTopicWords()
	{
		return topicWords;
	}

	public void setTopicWords(String[][] topicWords)
	{
		this.topicWords = topicWords;
	}

	public String[] getUnigramLabels()
	{
		return unigramLabels;
	}

	public void setUnigramLabels(String[] unigramLabels)
	{
		this.unigramLabels = unigramLabels;
	}

	public String[] getBigramLabels()
	{
		return bigramLabels;
	}

	public void setBigramLabels(String[] bigramLabels)
	{
		this.bigramLabels = bigramLabels;
	}

	public String[] getNounPhraseLabels()
	{
		return nounPhraseLabels;
	}

	public void setNounPhraseLabels(String[] nounPhraseLabels)
	{
		this.nounPhraseLabels = nounPhraseLabels;
	}
	
	
	
	public String[][] getYearUnigramLabels()
	{
		return yearUnigramLabels;
	}

	public void setYearUnigramLabels(String[][] yearUnigramLabels)
	{
		this.yearUnigramLabels = yearUnigramLabels;
	}

	public String[][] getYearBigramLabels()
	{
		return yearBigramLabels;
	}

	public void setYearBigramLabels(String[][] yearBigramLabels)
	{
		this.yearBigramLabels = yearBigramLabels;
	}

	public String[][] getYearNounPhraseLabels()
	{
		return yearNounPhraseLabels;
	}

	public void setYearNounPhraseLabels(String[][] yearNounPhraseLabels)
	{
		this.yearNounPhraseLabels = yearNounPhraseLabels;
	}

	public String[] getLabels(LabelType labelType)
	{
		switch (labelType)
		{
		case BIGRAM:
			return bigramLabels;
		case NOUN_PHRASE:
			return nounPhraseLabels;
		default:
			return unigramLabels;
		}
	}
	
	public String[][] getYearLabels(LabelType labelType)
	{
		switch (labelType)
		{
		case BIGRAM:
			return yearBigramLabels;
		case NOUN_PHRASE:
			return yearNounPhraseLabels;
		default:
			return yearUnigramLabels;
		}
	}	

	public List<DocumentInfo> getProposalInfo()
	{
		return proposalInfo;
	}

	public void setProposalInfo(List<DocumentInfo> proposalInfo)
	{
		this.proposalInfo = proposalInfo;
	}

	public List<DocumentInfo> getAllDocuments(int topic)
	{
		Map<DocumentInfo, Double> map = new HashMap<DocumentInfo, Double>();
		for (DocumentInfo proposal : proposalInfo)
		{
			if (proposal.topicProps != null)
			{
				if (proposal.topicProps[topic] > PROP_THRESHOLD || topic == getHighestTopic(proposal))
				{
					map.put(proposal, proposal.topicProps[topic]);
				}
			}
		}
		
		return Utils.sortByValue(map);
	}
	
	public List<DocumentInfo> getDocumentsByYear(int year, int topic)
	{
		Map<DocumentInfo, Double> map = new HashMap<DocumentInfo, Double>();
		for (DocumentInfo proposal : proposalInfo)
		{
			if (proposal.year == year && proposal.topicProps != null)
			{
				if (proposal.topicProps[topic] > PROP_THRESHOLD || topic == getHighestTopic(proposal))
				{
					map.put(proposal, proposal.topicProps[topic]);
				}
			}
		}
		
		return Utils.sortByValue(map);
	}
	
	private int getHighestTopic(DocumentInfo info)
	{
		double max = 0;
		int maxIndex = 0;
		for (int i = 0; i < info.topicProps.length; i++)
		{
			if (info.topicProps[i] > max)
			{
				max = info.topicProps[i];
				maxIndex = i;
			}
		}
		return maxIndex;
	}	
	
}
