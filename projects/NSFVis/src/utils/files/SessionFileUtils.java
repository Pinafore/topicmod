package utils.files;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import ontology.Constraint;
import utils.Constants;
import utils.LDAState;
import utils.PropertiesGetter;
import utils.Utils;

/**
 * Utilities for manipulating session files. Each session has a folder
 * associated with it, that contains:
 * - the output of the current LDA command
 * - model.lhood, which I call the "progress file" because it keeps track of
 * 		the progress of the LDA iterations. This is a file outputted by the
 * 		LDA command that contains one line per iteration, so it shows how many
 * 		iterations have completed so far.
 * - state.ser, containing the session's serialized LDAState object
 * - constraint-list.txt, containing a record of the past and current 
 * constraints
 * 
 */
public class SessionFileUtils
{
	public static void removeProgressFile(LDAState theState)
	{
		File file = new File(getProgressFile(theState));
		file.delete();
	}
	
	public static void copyProgressFile(LDAState theState) throws Exception
	{
		File currFile = new File(getProgressFile(theState));
		File oldFile = new File(getProgressFile(theState) + "old");
		if (currFile.exists())
		{
			DirectoryCopier.copyFolder(currFile, oldFile);
		}
		else
		{
			oldFile.delete();
		}
	}
	
	public static void createSessionDir(LDAState theState)
	{
		String sessionName = theState.sessionName;
		PropertiesGetter props = new PropertiesGetter();
		File file = new File(props.getSessionDir() + "/" + sessionName);
		file.mkdir();
		file = new File(props.getSessionDir() + "/" + sessionName + "/model_topic_assign");
		file.mkdir();
		file = new File(props.getSessionDir() + "/" + sessionName + "/model_topic_assign/assign");
		file.mkdir();
	}
	
	public static String getProgressFile(LDAState theState)
	{
		String sessionName = theState.sessionName;
		return (new PropertiesGetter().getSessionDir()) + "/" + sessionName + "/" + "model.lhood";
	}
	
	public static String getConstraintFile(LDAState theState)
	{
		String sessionName = theState.sessionName;
		return (new PropertiesGetter().getSessionDir()) + "/" + sessionName + "/constraints";
	}
	
	public static String getOutputDir(LDAState theState)
	{
		String sessionName = theState.sessionName;
		return (new PropertiesGetter().getSessionDir()) + "/" + sessionName + "/model_topic_assign/";
	}
	
	public static String getCommandOutputDir(LDAState theState)
	{
		String sessionName = theState.sessionName;
		return (new PropertiesGetter().getSessionDir()) + "/" + sessionName + "/model";	
	}
	
	public static void saveLDAState(LDAState state)
	{
		try
		{
			String sessionName = state.sessionName;
			String filename = (new PropertiesGetter().getSessionDir()) + "/" + sessionName + "/state.ser";
			FileOutputStream fos = new FileOutputStream(filename);
			ObjectOutputStream out = new ObjectOutputStream(fos);
			out.writeObject(state);
			out.close();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}
	
	public static LDAState loadLDAState(String sessionName)
	{
		try
		{
			String filename = (new PropertiesGetter().getSessionDir()) + "/" + sessionName + "/state.ser";
			FileInputStream fis = new FileInputStream(filename);
			ObjectInputStream in = new ObjectInputStream(fis);
			LDAState state = ((LDAState)in.readObject());
			in.close();
			return state;
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return null;
		}
	}
	
	public static void saveConstraints(LDAState theState)
	{
		try
		{
			String sessionName = theState.sessionName;
			String filename = (new PropertiesGetter().getSessionDir()) + "/" + sessionName + "/constraint-list.txt";
			BufferedWriter writer = new BufferedWriter(new FileWriter(filename, true));
			
			String str = "";
			for (Constraint constraint : theState.constraints)
			{
				str += "; ";
				if (constraint.isCannotLink)
					str += "(Cannot Link) ";
				else
					str += "(Must Link) ";
				str += Utils.listToString(constraint.words);
			}
			
			if (str.length() == 0)
			{
				str = "(None)";
			}
			else
			{
				str = str.substring(2);
			}
			
			str = theState.currentIterations + " iterations: " + str;
			writer.write(str + "\n");
			writer.close();
			
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}
	
	public static List<String> getSessionNames()
	{
		List<String> names = new ArrayList<String>();
		File file = new File(new PropertiesGetter().getSessionDir());
		names.addAll(Arrays.asList(file.list()));
		return names;
	}
}
