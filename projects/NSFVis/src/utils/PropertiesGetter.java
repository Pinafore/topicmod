package utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Properties;

/**
 * Class for reading various user-configurable properties from the
 * NSFVis.properties file.
 *
 */
public class PropertiesGetter 
{

	private String sessionDir;
	private String defaultDir;
	private String stopwordsLocation;
	private String updateCommand;
	private String ldaCommand;
	private String libraryPath;
	private String pythonPath;

	public PropertiesGetter() 
	{
		Properties properties = new Properties();
		try 
		{
			FileInputStream fis = new FileInputStream(Constants.PROPERTIES_FILE);
			properties.load(fis);
			sessionDir = properties.getProperty("sessionDir");
			defaultDir = properties.getProperty("defaultDir");
			stopwordsLocation = properties.getProperty("stopwordsLocation");
			updateCommand = properties.getProperty("updateCommand");
			ldaCommand = properties.getProperty("ldaCommand");
			libraryPath = properties.getProperty("libraryPath");
			pythonPath = properties.getProperty("pythonPath");
			fis.close();
		} 
		catch (IOException e) 
		{
			File file = new File(Constants.PROPERTIES_FILE);
			System.out.println("No props file at: " + file.getAbsolutePath());
			e.printStackTrace();
		}

	}

	public String getSessionDir() {
		return sessionDir;
	}

	public String getDefaultDir() {
		return defaultDir;
	}

	public String getStopwordsLocation() {
		return stopwordsLocation;
	}

	public String getUpdateCommand() {
		return updateCommand;
	}

	public String getLdaCommand() {
		return ldaCommand;
	}

	public String getLibraryPath() {
		return libraryPath;
	}

	public String getPythonPath() {
		return pythonPath;
	}
	
	public static void main(String[] args)
	{
		System.out.println(new PropertiesGetter().getPythonPath());
	}
	

}

