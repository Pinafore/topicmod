package servlets;

import java.io.IOException;
import java.util.List;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import lda.external.LDACaller;
import lda.post.RawTopicInfo;
import lda.post.TopicInfoGetter;

import org.json.JSONArray;
import org.json.JSONObject;

import utils.Colors;
import utils.Constants;
import utils.DataSet;
import utils.DocumentInfo;
import utils.LDAState;
import utils.LabelType;
import utils.TopicInfo;
import utils.files.DefaultFileUtils;
import utils.files.SessionFileUtils;

/**
 * Servlet for checking the status of an LDA run. This is called through
 * AJAX when polling for LDA completion.
 */
public class CheckLDAServlet extends HttpServlet {
	private static final long serialVersionUID = 1L;
       
    /**
     * @see HttpServlet#HttpServlet()
     */
    public CheckLDAServlet() {
        super();
        // TODO Auto-generated constructor stub
    }

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doGet(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException
	{
		process(request, response);
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doPost(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException
	{
		process(request, response);
	}
	
	private void process(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException
	{	
		try
		{
			// Get the LDA state from the session
			LDAState theState = (LDAState)request.getSession().getAttribute("state");
			
			int numIterations = theState.newIterations;
			
			int numLines = 0;
			String command = "";
			
			// Are we running any additional LDA iterations? numIterations 
			// will be 0 if we're just updating something that doesn't require
			// calling the LDA program, such as the topic display type. 
			if (numIterations > 0)
			{
				// Use the progress files ("model.lhood") to figure out how
				// many iterations have been run so far.
				numLines = LDACaller.getLines(SessionFileUtils.getProgressFile(theState));
				int existingLines = LDACaller.getLines(SessionFileUtils.getProgressFile(theState) + "old");
				numLines -= existingLines; // the lines that are already there
				
				if (numLines > numIterations)
					numLines = numIterations;
				
				command = theState.lastCommand;
			}
			boolean done = (numLines >= numIterations);
			
			// Return a JSON object to the user
			JSONObject obj = new JSONObject();
			obj.put("numIterations", numLines);
			obj.put("totalIterations", numIterations);
			obj.put("command", command);
			obj.put("version", "0");
			obj.put("done", done);
			
			if (done)
			{
				DataSet dataSet = theState.dataSet;
				LabelType labelType = theState.labelType;
				int randomSeed = theState.randomSeed;
				int numTopics = theState.numTopics;
				
				LDACaller.Result result = LDACaller.getResult(SessionFileUtils.getOutputDir(theState),
						Constants.OUTPUT_INDEX);

				// Parse the document info from the result
				List<DocumentInfo> propInfo = DocumentInfo.getFromResult(result, dataSet.years);
				
				// set the topic info
				RawTopicInfo rawInfo = new RawTopicInfo(result, dataSet);
				TopicInfo newTopicInfo = TopicInfoGetter.getTopicInfo(dataSet,
						rawInfo, propInfo, dataSet.years,
						labelType);
				theState.topicInfo = newTopicInfo;

				TopicInfo topicInfo = theState.topicInfo;

				// Get the information specifically needed by the GUI, and send it
				// to the user in JSON form.
				JSONArray array = new JSONArray();
				for (int year : dataSet.years)
					array.put(year);
				obj.put("years", array);
				
				obj.put("numTopics", topicInfo.getNumTopics());

				String[] topicNames = topicInfo.getLabels(labelType);
				obj.put("topicNames", JSONArrayConverter.toArray(topicNames));

				String[] colors = Colors.getColors(topicInfo.getNumTopics());
				obj.put("colors", JSONArrayConverter.toArray(colors));

				int[][] gridData = topicInfo.getTopicOrder();
				obj.put("gridData", JSONArrayConverter.to2DArray(gridData));

				double[][] stackData = topicInfo.getYearProps();
				obj.put("stackData", JSONArrayConverter.to2DArray(stackData));
				
				if (numIterations > 0)
				{
					// reset the number of iterations
					theState.currentIterations += theState.newIterations;
					theState.newIterations = 0;
					theState.dirty = false;

					// record the constraints
					SessionFileUtils.saveConstraints(theState);
				}
				
				// Save the current session.
				SessionFileUtils.saveLDAState(theState);

				// If the random seed still hasn't been set and some iterations
				// were run - which can only happen in an initial LDA without
				// constraints - see if we need to save the result as a default 
				// file.
				if (randomSeed < 0 && numIterations > 0)
				{
					DefaultFileUtils.trySaveFile(
							theState.sessionName, dataSet, numTopics);
				}

				
			}
		
			response.getWriter().write(obj.toString());
			response.setContentType("application/json");
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}


}
