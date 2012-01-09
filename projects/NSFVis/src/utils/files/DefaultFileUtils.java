package utils.files;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import utils.Constants;
import utils.DataSet;
import utils.PropertiesGetter;

/**
 * Methods for creating and loading the default files. These files store the
 * results of the initial LDA for each (data set, number of topics) pair. At
 * the beginning of a session, we first try to find a default file for the
 * selected data set and number of topics. If one is found, we load it
 * instead of performing LDA. Otherwise, we perform LDA and save the results
 * as a new default file.
 */
public class DefaultFileUtils
{
	static
	{
		loadList();
	}
	
	private static List<DefaultFile> theList;
	
	static class DefaultFile
	{
		String directory;
		String dataSetName;
		int numTopics;
	}
	
	/**
	 * Load a list of available default files, with their data sets
	 * and numbers of topics. 
	 */
	private static void loadList()
	{
		theList = new ArrayList<DefaultFile>();
		String parentDir = new PropertiesGetter().getDefaultDir();
		File[] childDirs = new File(parentDir).listFiles();
		for (File childDir : childDirs)
		{
			String[] parts = childDir.getName().split("_");
			DefaultFile file = new DefaultFile();
			file.directory = childDir.getPath();
			file.dataSetName = parts[0];
			file.numTopics = Integer.parseInt(parts[1]);
			theList.add(file);
		}
	}
	
	/**
	 * Try to load a default file with the given data set and
	 * number of topics, and copy it into the user's session
	 * directory.
	 * @return true if this was successful, and false otherwise
	 */
	public static boolean tryLoadFile(String sessionName, DataSet dataSet, int numTopics)
	{
		loadList();
		String dsName = sanitizeName(dataSet.name);
		DefaultFile defaultFile = null;
		
		for (DefaultFile file : theList)
		{
			if (file.dataSetName.equals(dsName) && file.numTopics == numTopics)
			{
				// found a matching default file
				defaultFile = file;
			}
		}
		
		// if we didn't find a file
		if (defaultFile == null)
		{
			return false;
		}
		
		PropertiesGetter props = new PropertiesGetter();
		File storageDir = new File(props.getSessionDir() + "/" + sessionName);
		File defaultDir = new File(props.getDefaultDir() + "/" + dsName + "_" + numTopics);
		
		try
		{
			DirectoryCopier.copyFolder(defaultDir, storageDir);
			return true;
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return false;
		}
	}
	
	/**
	 * After running the initial LDA, check to see if a default file exists
	 * for this data set and number of topics. If not, save one.
	 * @return true if a default file was saved, false if not (either
	 * because the file already existed or because of an error)
	 */
	public static boolean trySaveFile(String sessionName, DataSet dataSet, int numTopics)
	{
		String dsName = sanitizeName(dataSet.name);
		
		for (DefaultFile file : theList)
		{
			if (file.dataSetName.equals(dsName) && file.numTopics == numTopics)
			{
				// we don't need to save the file because it already exists
				return false;
			}
			
		}
		
		PropertiesGetter props = new PropertiesGetter();

		File oldDir = new File(props.getSessionDir() + "/" + sessionName);		
		File newDir = new File(props.getDefaultDir() + "/" + dsName + "_" + numTopics);
		newDir.mkdir();
		
		try
		{
			DirectoryCopier.copyFolder(oldDir, newDir);
			loadList();
			return true;
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return false;
		}
	}
	
	/**
	 * Delete spaces and underscores in the data set name.
	 */
	private static String sanitizeName(String name)
	{
		return name.replace(" ", "").replace("_", "");
	}
}
