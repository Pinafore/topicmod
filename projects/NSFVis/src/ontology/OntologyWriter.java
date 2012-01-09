package ontology;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;

import topicmod_projects_ldawn.WordnetFile.WordNetFile;
import topicmod_projects_ldawn.WordnetFile.WordNetFile.Synset;
import topicmod_projects_ldawn.WordnetFile.WordNetFile.Synset.Word;
import utils.Utils;


/**
 * Converts a set of user-selected constraints into Protocol Buffer form.
 * This is an adaptation of Yuening's Python code that does the same thing.
 *  
 */
public class OntologyWriter
{
	private Map<Integer, Set<Integer>> parents; 
	
	private int numFiles;
	private String filename;
	
	private Map<Integer, Synset.Builder> synsets;
	
	private WordNetFile.Builder wordnet;
	
	private int root;
	private Map<Integer, Map<String, Integer>> vocab;
	private boolean propagateCounts;
	
	static class WordTuple
	{
		public int language;
		public String word;
		public int count;
	}
	
	static class VocabEntry
	{
		public int index;
		public int language;
		public int flag;	
	}
	
	final static int ENGLISH_ID = 0;
	
	private OntologyWriter(String filename, boolean propagateCounts)
	{
		this.filename = filename;
		this.propagateCounts = propagateCounts;
		
		vocab = new TreeMap<Integer, Map<String, Integer>>();
		synsets = new TreeMap<Integer, Synset.Builder>();
		parents = new TreeMap<Integer, Set<Integer>>();
		
		wordnet = WordNetFile.newBuilder();
		wordnet.setRoot(-1);
		
		root = -1;
	}	
	
	private void addParent(int childId, int parentId)
	{
		if (!parents.containsKey(childId))
		{
			parents.put(childId, new TreeSet<Integer>());
		}
		parents.get(childId).add(parentId);
	}
	
	private void addSynset(int numericId, String senseKey, List<Integer> children,
			List<WordTuple> words)
	{		
		Synset.Builder synset = Synset.newBuilder();
		int rawCount = 0;
		synset.setOffset(numericId);
		synset.setKey(senseKey);
		
		for (int child : children)
		{
			addParent(child, numericId);
			synset.addChildrenOffsets(child);
		}
		
		for (WordTuple tuple : words)
		{
			Word.Builder word = Word.newBuilder();
			word.setLangId(tuple.language);
			word.setTermId(getTermId(tuple.language, tuple.word));
			word.setTermStr(tuple.word);
			word.setCount(tuple.count);
			rawCount += tuple.count;
			synset.addWords(word);
		}
		
		synset.setRawCount(rawCount);
		synset.setHyponymCount(rawCount);		
		
		wordnet.addSynsets(synset);
		synsets.put(numericId, synset);
	}

	private int getTermId(int language, String term)
	{
		if (!vocab.containsKey(language))
		{
			vocab.put(language, new TreeMap<String, Integer>());
		}
		if (!vocab.get(language).containsKey(term))
		{
			int length = vocab.get(language).size();
			vocab.get(language).put(term, length);
		}
		return vocab.get(language).get(term);
	}
	  
	private void findRoot(Map<Integer, Synset.Builder> synsets)
	{
		for (int synsetId : synsets.keySet())
		{
			if (synsetId % 1000 == 0)
			{
				System.out.println("Finalizing " + synsetId);
			}
			for (int parentId : getParents(synsetId))
			{
				if (propagateCounts)
				{
					double hypCount = synsets.get(parentId).getHyponymCount();
					double rawCount = synsets.get(synsetId).getRawCount();
					synsets.get(parentId).setHyponymCount(hypCount + rawCount);
				}
			}
		}
	}
	
	private void write(WordNetFile.Builder wnFile)
	{
		try
		{
			String newFilename = filename + "." + numFiles;
			WordNetFile builtFile = wnFile.build();
			builtFile.writeTo(new FileOutputStream(newFilename));
			System.out.println("Serialized version written to: " + newFilename);
			this.numFiles ++;
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}
	
	// Named this so it doesn't conflict with Object.finalize
	private void finalizeMe()
	{
		findRoot(synsets);
		
		if (root < 0)
			System.out.println("No root has been found!");
		wordnet.setRoot(root);
		write(wordnet);
		
	}
	
	private List<Integer> getParents(int id)
	{
		List<Integer> parentList = new ArrayList<Integer>();
		if (!parents.containsKey(id) || parents.get(id).size() == 0)
		{
			if (root < 0)
				root = id;
			return new ArrayList<Integer>();
		}
		else
		{
			parentList.addAll(parents.get(id));
			for (int parentId : parents.get(id))
			{
				parentList.addAll(getParents(parentId));
			}
		}
		return parentList;
		
	}
	
	private static List<Set<String>> mergeConstraints(List<List<String>> constraints)
	{
		List<Set<String>> mergedConstr = new ArrayList<Set<String>>();
		Map<String, Integer> mergeDict = new HashMap<String, Integer>();
		int mergeIndex = -1;
		
		for (List<String> constraint : constraints)
		{
			int index = -1;
			for (String word : constraint)
			{
				if (mergeDict.containsKey(word))
				{
					index = mergeDict.get(word);
					break;
				}
			}
			
			if (index == -1)
			{
				mergeIndex ++;
				mergedConstr.add(new HashSet<String>());
				index = mergeIndex;
			}
			
			for (String word : constraint)
			{
				mergedConstr.get(index).add(word);
				if (!mergeDict.containsKey(word))
				{
					mergeDict.put(word,index);
				}
			}
		}
		return mergedConstr;
	}
	
	private static List<List<String>> getMustLinkConstraints(List<Constraint> consList)
	{
		List<List<String>> wordList = new ArrayList<List<String>>();
		for (Constraint constr : consList)
		{
			if (!constr.isCannotLink)
			{
				wordList.add(constr.words);
			}
		}
		return wordList;
	}
	
	private static Map<String, VocabEntry> getVocab(String filename)
	{
		Map<String, VocabEntry> vocab = new TreeMap<String, VocabEntry>();
		int index = 0;
		try
		{
			List<String> lines = Utils.readAll(filename);
			for (String line : lines)
			{
				String[] words = line.trim().split("\t");
				// 3D features for each word [index, language, flag]
     	        // index for tree node index
     	        // language: support multiple languages
     	        // flag: initial -1; in constrain 1; no constrain but used: 0;
				VocabEntry vocabEntry = new VocabEntry();
				vocabEntry.index = index;
				vocabEntry.language = Integer.parseInt(words[0]);
				vocabEntry.flag = -1;
				vocab.put(words[1], vocabEntry);
				index++;
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		return vocab;
	}
	
	private static boolean checkConstraints(List<Constraint> constraints, Map<String, VocabEntry> vocab )
	{
		for (Constraint constraint : constraints)
		{
			for (String word : constraint.words)
			{
				if (!vocab.containsKey(word))
					return false;
			}
		}
		return true;
	}
	
	private static Constraint makeConstraint(boolean cannotLink, String[] array)
	{
		Constraint constr = new Constraint();
		constr.isCannotLink = cannotLink;
		constr.words = new ArrayList<String>(Arrays.asList(array));
		constr.words = constr.words.subList(0,2);
		return constr;
	}
	
	/**
	 * This is the top-level method that creates the ontology from a set of
	 * Constraint objects.
	 * @param vocabFilename the .voc file corresponding to the data set
	 * being used
	 */
	public static void createOntology(List<Constraint> constraints, String vocabFilename,
			String outputDir)
	{
	    Map<String, VocabEntry> vocab = getVocab(vocabFilename);
	    OntologyWriter writer = new OntologyWriter(outputDir, false);
	    if (!checkConstraints(constraints, vocab))
	    {
	    	System.out.println("Constraints had words not in vocab!");
	    	return;
	    	
	    }
	    
	    List<List<String>> mustLink = getMustLinkConstraints(constraints);
    	List<Set<String>> mergedConstraints = mergeConstraints(mustLink);
	    int nodeIndex = 1;
	    List<Integer> rootChildren = new ArrayList<Integer>();
	    
	    for (Set<String> constraint : mergedConstraints)
	    {
	    	List<WordTuple> words = new ArrayList<WordTuple>();
	    	for (String term : constraint)
	    	{
	    		WordTuple tuple = new WordTuple();
	    		tuple.count = 1;
	    		tuple.language = ENGLISH_ID;
	    		tuple.word = term;
	    		words.add(tuple);
	    		vocab.get(term).flag = 1;
	    	}
	    	writer.addSynset(nodeIndex, "constraint" + nodeIndex, 
	    			new ArrayList<Integer>(), words);
	    	rootChildren.add(nodeIndex);
	    	nodeIndex++;
	    }
	    
	    // Unused words
	    for (String word : vocab.keySet())
	    {
	    	if (vocab.get(word).flag == -1)
	    	{
	    		rootChildren.add(nodeIndex);
	    		WordTuple tuple = new WordTuple();
	    		tuple.count = 1;
	    		tuple.language = ENGLISH_ID;
	    		tuple.word = word;
	    		writer.addSynset(nodeIndex, word, new ArrayList<Integer>(), 
	    				Collections.singletonList(tuple));
	    		vocab.get(word).flag = 0;
	    		nodeIndex++;
	    	}
	    }
	    
	    writer.addSynset(0, "root", rootChildren, new ArrayList<WordTuple>());
	    writer.finalizeMe();
	}
	
	public static void main(String[] args)
	{
		String vocabFn = "/nfshomes/bsonrisa/lda/data/20news/20_news.voc";
			//"C:/corpus/constraint-test/nsf.voc";
		String outputFn = "/nfshomes/bsonrisa/output";
			//"C:/corpus/constraint-test/output";
		
	    List<Constraint> constraints = new ArrayList<Constraint>();
	    
	    constraints.add(makeConstraint(false, new String[]{"catholic","scripture","clh","resurrection","mary","pope","marriage","absolute","worship","revelation","sabbath","hold","spiritual","romans","married","grace","pray","divine","suggest","doctrine","orthodox"}));
	    constraints.add(makeConstraint(false, new String[]{"sale","offer","shipping","cover","excellent","condition","art","included","wolverine","hulk","trade","annual","brand","picture","ghost","perfect","liefeld","sabretooth","hobgoblin","complete","room"}));
	    constraints.add(makeConstraint(false, new String[]{"stephanopoulos","tax","cramer","myers","clayton","sexual","south","secretary","continue","education","russia","economic","homosexual","george","plan","school","summer","male","hitler","senior","official"}));
	    constraints.add(makeConstraint(false, new String[]{"jpeg","gif","processing","sgi","quality","animation","convert","tiff","analysis","ray","formats","site","polygon","quicktime","visualization","interactive","shareware","conversion","siggraph","rayshade","viewer"}));
	    constraints.add(makeConstraint(false, new String[]{"judas","kent","context","tyre","hudson","lds","mormon","stephen","testament","kendig","specifically","salvation","kingdom","followers","malcolm","ksand","homosexuality","born","alink","tony","cult"}));
	    constraints.add(makeConstraint(false, new String[]{"widget","motif","entry","xterm","xlib","resource","visual","mit","event","client","null","int","contest","default","char","colormap","string","oname","toolkit","function","expose"}));
	    constraints.add(makeConstraint(false, new String[]{"gun","guns","firearms","militia","weapon","handgun","firearm","safety","assault","compound","cdt","atf","criminal","roby","waco","nra","violent","dangerous","gang","homicide","weaver"}));
	    constraints.add(makeConstraint(false, new String[]{"atheism","keith","islam","atheist","islamic","livesey","murder","jaeger","motto","fallacy","definition","qur","position","liar","innocent","jon","gregg","bobby","satan","rushdie","mozumder"}));
	    constraints.add(makeConstraint(false, new String[]{"medical","msg","food","disease","doctor","cancer","gordon","banks","pain","medicine","geb","treatment","candida","dyer","diet","weight","skepticism","normal","intellect","chastity","effects"}));
	    constraints.add(makeConstraint(false, new String[]{"encryption","clipper","privacy","secure","nsa","escrow","des","enforcement","crypto","communications","pgp","ripem","mov","cryptography","rsa","random","trust","wiretap","proposal","scheme","anonymity"}));
	    constraints.add(makeConstraint(false, new String[]{"nasa","launch","lunar","shuttle","satellite","moon","orbit","spacecraft","flight","mission","henry","solar","station","rocket","mars","pat","development","planetary","orbital","propulsion","baalke"}));
	    constraints.add(makeConstraint(false, new String[]{"apple","quadra","nubus","duo","centris","fpu","vram","simm","macintosh","powerpc","powerbook","processor","iisi","lciii","slot","ethernet","socket","option","due","connect","accelerator"}));
	    constraints.add(makeConstraint(false, new String[]{"hockey","nhl","period","cup","goal","wings","chi","puck","montreal","det","bos","calgary","ice","playoffs","van","stanley","cal","tor","pit","pick","lemieux"}));
	    constraints.add(makeConstraint(false, new String[]{"bios","isa","adaptec","transfer","dma","vlb","boot","feature","jumper","irq","esdi","eisa","backup","cmos","wayne","wait","mfm","seagate","boards","slave","pro"}));
	    constraints.add(makeConstraint(false, new String[]{"pitching","alomar","morris","sox","ball","mets","phillies","rbi","lopez","hitter","career","hall","offense","stats","clemens","baerga","houston","hitting","atlanta","era","pitch"}));
	    constraints.add(makeConstraint(false, new String[]{"israel","turkish","armenian","israeli","armenians","armenia","arab","turkey","genocide","lebanese","land","soviet","azerbaijan","palestinian","troops","lebanon","serdar","henrik","argic","azerbaijani","nazi"}));
	    constraints.add(makeConstraint(false, new String[]{"max","bhj","microsoft","swap","risc","allocation","instruction","borland","paradox","latest","cross","select","editor","desktop","novell","load","apps","truetype","tel","deskjet","cica"}));
	    constraints.add(makeConstraint(false, new String[]{"cars","ford","callison","auto","brake","toyota","sho","wheel","craig","models","automatic","mustang","owner","mileage","brakes","transmission","blah","taurus","wagon","stupid","stick"}));
	    constraints.add(makeConstraint(false, new String[]{"bike","dod","ride","motorcycle","riding","bmw","helmet","dog","behanna","ama","lock","fit","chris","nick","yamaha","biker","hey","harley","tank","countersteering","jacket"}));
	    constraints.add(makeConstraint(false, new String[]{"wire","circuit","wiring","voltage","neutral","amp","hot","audio","electrical","electronics","grounding","heat","equipment","detector","led","cooling","signal","outlet","metal","conductor","panel"}));

	    createOntology(constraints, vocabFn, outputFn);
	}

}
