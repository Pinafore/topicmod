package utils;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
 * Each of these entries represents a data set. Update this file when
 * adding a new data set.
 */
public class DataSet implements Serializable
{
	protected final static long serialVersionUID = 2;
	
	/**
	 * The name of the data set
	 */
	public String name;
	
	/**
	 * The file containing the list of noun phrases for this data set.
	 * You need to generate this file offline because it takes a long time.
	 */	
	public String nounPhraseFilename;
	
	/**
	 * The vocabulary (.voc) file for this data set. Used when calling LDA.
	 */
	public String vocabFilename;
	
	/**
	 * The protocol buffer index file for this data set.
	 */
	public String protocolFilename;
	
	/**
	 * The years appearing in this data set.
	 */
	public List<Integer> years;
	
	/**
	 * True if the index file contains absolute (rather than relative) paths
	 * to the document protocol buffers.
	 */
	public boolean useAbsolutePath;
	
	public DataSet()
	{
		years = new ArrayList<Integer>();
	}

	public static List<DataSet> getDataSets()
	{
		List<DataSet> retval = new ArrayList<DataSet>();

		try
		{
			// read the CSV file
			List<String> lines = Utils.readAll(Constants.DATASET_FILE);
			for (String line : lines)
			{
				String[] parts = line.split(",");
				DataSet ds = new DataSet();
				ds.name = parts[0];
				ds.protocolFilename = parts[1];
				ds.vocabFilename = parts[2];
				ds.nounPhraseFilename = parts[3];
				ds.useAbsolutePath = Boolean.parseBoolean(parts[4]);
				
				for (int i = 5; i < parts.length; i++)
					ds.years.add(Integer.parseInt(parts[i]));
				
				retval.add(ds);
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		return retval;
		
	}
	
	/**
	 * Extract the link (if any) from the title of the document, according
	 * to the data set. 
	 */
	public static String getLink(DataSet dataSet, String title)
	{
		if (dataSet.name.equals("Small NSF Dataset"))
		{
			String[] parts = title.split("_");
			return "http://www.nsf.gov/awardsearch/showAward.do?AwardNumber=" + parts[0];
		}
		else
		{
			// default: no link
			return null;
		}
	}
	
	/**
	 * Extract the part to display on the visualization page from the 
	 * title of the document, according to the data set.
	 */
	public static String getDisplayTitle(DataSet dataSet, String title)
	{
		if (dataSet.name.equals("Small NSF Dataset"))
		{
			String[] parts = title.split("_");
			return parts[1];
		}
		else if (dataSet.name.equals("NY Times"))
		{
			String[] parts = title.split(".xml ");
			if (parts.length >= 2)
				return parts[1];
			return title;
		}
		else
		{
			// default: return the whole document title
			return title;
		}
	}
}

