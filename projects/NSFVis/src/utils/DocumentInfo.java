package utils;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import lda.external.LDACaller;
import lib_corpora_proto.ProtoCorpus.Document;

/**
 * The properties of a single document.
 */
public class DocumentInfo implements Serializable
{
	public String id;
	public String title;
	public int year;
	
	/**
	 * The proportion of words for each topic in this document. 
	 */
	public double[] topicProps;
	
	protected final static long serialVersionUID = 2;
	
	/**
	 * Extract DocumentInfo information for each document from the protocol
	 * buffer returned from the LDA command.
	 */
	public static List<DocumentInfo> getFromResult(LDACaller.Result result, 
			List<Integer> years)
	{
		List<DocumentInfo> retval = new ArrayList<DocumentInfo>();
		for (Document document : result.documents)
		{
			DocumentInfo pInfo = new DocumentInfo();
			pInfo.id = "" + document.getId();
			pInfo.title = document.getTitle();
			
			int year = document.getDate().getYear();
			int yearIndex = years.indexOf(year);
			if (yearIndex < 0)
				yearIndex = 0;
			
			pInfo.year = yearIndex;
			retval.add(pInfo);
		}
		return retval;
	
	}
	
}
