package ontology;

import java.io.Serializable;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Class that contains the contents and manipulation methods for a single
 * must-link or cannot-link constraint.
 *
 */
public class Constraint implements Serializable
{
	private static final long serialVersionUID = 1;
	
	/**
	 * True if this is a cannot-link constraint, false if this is a
	 * must-link.
	 */
	public boolean isCannotLink;
	
	/**
	 * List of words (in no particular order) in this constraint.
	 */
	public List<String> words;
	
	/**
	 * Gets a set of all the words contained in a list of constraints,
	 * optionally excluding the constraint at a given index (editIndex).
	 */
	public static Set<String> getAllWords(List<Constraint> cons, int editIndex)
	{
		Set<String> words = new HashSet<String>();
		for (int i = 0; i < cons.size(); i++)
		{
			if (i != editIndex)
			{
				words.addAll(cons.get(i).words);
			}
		}
		return words;
	}
	
	/**
	 * Converts the constraint at (index) position in (list) constraint list
	 * into a Javascript array.
	 */
	public static String getJavascript(List<Constraint> list, int index)
	{
		if (index < 0)
			return "new Array()";
		else
		{
			String str = "";
			for (String word : list.get(index).words)
			{
				str += ",'" + word + "'";
			}
			return "[" + str.substring(1) + "]";
		}
	}
}
