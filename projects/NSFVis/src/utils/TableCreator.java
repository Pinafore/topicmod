package utils;

import java.util.List;

/**
 * Dynamically creates the HTML for the legend and topic-year tables on the
 * visualization page. JavaScript properties such as 'onClick' and CSS classes
 * are added here too, so keep this class in sync with the JS and CSS files.
 *
 */
public class TableCreator
{
	/**
	 * Creates the legend table. 
	 */
	public static String getLegend(int numTopics, LabelType labelType)
	{
		String str = "<div class='legendTable'>";
		str += "<div class='legendTR'><div class='legendHead'>Topic Name</div>";
		str += "<div class='legendHead'>Hidden</div></div>";
		
		for (int i = 0; i < numTopics; i++)
		{
			str += "<div class='legendTR'>";
			str += "<div class='outerLegendTD' onClick='selectTopic(" + i + ")'" +
				" name='legend" + i + "'><div class='legendTD'></div></div>";
			str += "<div class='legendToggle'><input type='checkbox' onClick='toggleTopic(" + i + 
				")'/></div>";
			str += "</div>";
		}
		str += "</div>";
		return str;
		
	}
	
	/**
	 * Creates the topic-year grid. 
	 */	
	public static String getGrid(int numTopics, List<Integer> years)
	{
		String str = "<div class='gridTable'>";
		
		for (int j = 0; j < years.size(); j++)
		{	
			str += "<div class='gridTR'>";
			for (int i = 0; i < numTopics; i++)
			{
				String cellName = "topic" + i + "year" + j;
				str += "<div class='outerGridTD' name='" + cellName + "'" + 
					" onClick = 'selectTopicFromGrid(" + i + "," + j + ");'>" +
					" <div class='gridTD'>Topic</div></div>";
			}
			str += "</div>";
		}
		
		str += "</div>";
		
		str += "<div class='gridYears'>";
		for (int j = 0; j < years.size(); j++)
		{
			int year = years.get(j);
			str += "<div class='gridYear'>" + year + "</div>";
		}
		str += "</div>";
		
		return str;
	}

}
