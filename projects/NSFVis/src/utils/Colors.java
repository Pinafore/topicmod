package utils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * The color scheme for the visualization. The colors roll over after 20
 * topics. If you can find a good color scheme with more than 20 easily
 * distinguishable colors, please change it!
 */
public class Colors
{
	private final static String[] colors = {
		
		"#66FFFF", "#CC9999", "#66FF66", "#FF66B3",
		"#FFFF33", "#FF6666", "#9999FF", "#B3FF66",
		"#99CCFF", "#FFB366", "#CC99FF", "#EB9C00",
		"#FF9999", "#FFFF99", "#6699FF", "#CCFF99",
		"#99FF99", "#AAAAAA", "#FF99FF", "#4F9C9C"

	};
	private final static List<String> colorList = Arrays.asList(colors);
	
	public static String[] getColors(int numTopics)
	{
		List<String> allColors = new ArrayList<String>();
		while (allColors.size() < numTopics)
			allColors.addAll(colorList);
		return allColors.toArray(new String[0]);
	}
	
	

}
