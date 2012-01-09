package utils;

/**
 * Enumeration of topic labeling types.
 */
public enum LabelType
{
	UNIGRAM("Unigram (one word)"), 
	BIGRAM("Bigram (two words)"), 
	NOUN_PHRASE("Noun phrase");
	
	private String displayName;
	
	private LabelType(String name)
	{
		this.displayName = name;
	}
	
	public String getDisplayName()
	{
		return displayName;
	}
}
