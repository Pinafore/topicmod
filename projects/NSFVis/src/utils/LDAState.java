package utils;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import ontology.Constraint;

/**
 * Stores various pieces of state information for a given session.
 */
public class LDAState implements Serializable
{
	private static final long serialVersionUID = 2;
	
	/**
	 * User-selected number of topics
	 */
	public int numTopics = 0;
	
	/**
	 * User-selected data set
	 */
	public DataSet dataSet = null;
	
	/**
	 * User-selected topic labeling type. Unlike number of topics and data set,
	 * this can be changed without re-running LDA.
	 */
	public LabelType labelType = null;
	
	/**
	 * Current set of selected constraints
	 */
	public List<Constraint> constraints = new ArrayList<Constraint>();
	
	/**
	 * LDA iterations run so far (gets reset when 'Reset Topics' is clicked)
	 */
	public int currentIterations = 0;
	
	/**
	 * LDA iterations to run in the next LDA command. This is the *additional* 
	 * number of iterations, not the *total*.
	 */
	public int newIterations = 0;
	
	/**
	 * True if we're updating an existing LDA run, false if we're startign over.
	 */
	public boolean append = false;
	
	// a negative random seed means that we should use the default
	public int randomSeed = -1;
	
	/**
	 * Last TopicInfo object to be outputted.
	 */
	public transient TopicInfo topicInfo = null;
	
	/**
	 * Last LDA command run.
	 */
	public transient String lastCommand = "";
	
	/**
	 * Dirty flag: true if we've changed the constraints in any way
	 * without running LDA to refine the topics.
	 */
	public transient boolean dirty = false;
	
	/**
	 * Session name
	 */
	public String sessionName = null;
	
	/**
	 * Remove all constraints
	 */
	public void resetConstraints()
	{
		constraints = new ArrayList<Constraint>();
	}
	
	/**
	 * Add a constraint
	 */
	public void addConstraint(List<String> words, boolean isCannotLink)
	{
		Constraint constr = new Constraint();
		constr.isCannotLink = isCannotLink;
		constr.words = words;
		constraints.add(constr);
		dirty = true;
	}
	
	/**
	 * Replace a constraint with a new set of words
	 */
	public void replaceConstraint(int index, List<String> words, boolean isCannotLink)
	{
		Constraint constr = new Constraint();
		constr.isCannotLink = isCannotLink;
		constr.words = words;
		constraints.set(index, constr);
		dirty = true;
	}
	
	/**
	 * Delete a constraint
	 */
	public void deleteConstraint(int index)
	{
		if (index < constraints.size())
		{
			constraints.remove(index);
		}
		dirty = true;
	}

	
}
