package servlets;

import java.io.IOException;
import java.util.Random;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import utils.Constants;
import utils.DataSet;
import utils.LDAState;
import utils.LabelType;
import utils.files.DefaultFileUtils;
import utils.files.SessionFileUtils;

/**
 * Servlet that gets called when a user starts a new session, loads
 * an existing session. Creates or loads the session state and session
 * directory, and loads 
 */
public class InitServlet extends HttpServlet
{
	private static final long serialVersionUID = 1L;

	/**
	 * @see HttpServlet#HttpServlet()
	 */
	public InitServlet()
	{
		super();
		// TODO Auto-generated constructor stub
	}

	private void process(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException
	{	
		// LDA state variable (we'll fill this in later)
		LDAState theState = null;
		
		// Are we resetting or continuing the iterations?
		boolean resetIters = false;
		boolean resetConstraints = false;
		boolean continueIters = false;
		
		// are we loading an existing session? if so, want to disable some things
		String sessionType = request.getParameter("sessionType");
		boolean loadingSession = (sessionType != null && sessionType.equals("load"));
		boolean creatingSession = (sessionType != null && sessionType.equals("new"));
		
		// set the reset/continue parameters based on things passed in
		if (request.getParameter("reset") != null)
		{
			resetIters = true;
		}
		if (request.getParameter("resetConstraints") != null)
		{
			resetConstraints = true;
		}		
		if (request.getParameter("continue") != null)
		{
			continueIters = true;
		}
		
		// are we creating a new session?
		if (creatingSession)
		{
			// create the LDA state, and set the session variable
			theState = new LDAState();
			request.getSession(true).setAttribute("state", theState);
			
			String sessionName = request.getParameter("newSessionName");
			theState.sessionName = sessionName.replace(" ", "_"); // get rid of spaces
			SessionFileUtils.createSessionDir(theState);
			
			// Create the data set from the servlet parameters
			DataSet dataSet = new DataSet();
			
			// Did they choose a data set number?
			if (request.getParameter("dataSet") != null)
			{
				dataSet = DataSet.getDataSets().get(Integer.parseInt(request.getParameter("dataSet")));
			}
			// Or did they specify the parameters manually?
			else
			{
				dataSet.name = request.getParameter("dsName");
				dataSet.protocolFilename = request.getParameter("protocolFilename");
				dataSet.vocabFilename = request.getParameter("vocabFilename");
				dataSet.nounPhraseFilename = request.getParameter("nounPhraseFilename");	
				dataSet.useAbsolutePath = Boolean.parseBoolean(request.getParameter("useAbsolutePath"));
				
				String[] years = request.getParameter("years").split(",");
				for (String year : years)
					dataSet.years.add(Integer.parseInt(year));
				
			}
			
			int numTopics = Integer.parseInt(request.getParameter("numTopics"));
		
			// set the static variables
			theState.dataSet = dataSet;
			theState.numTopics = numTopics;
			
			// set a default label type (UNIGRAM)
			theState.labelType = LabelType.UNIGRAM;
			
			// try to load a default file
			if (DefaultFileUtils.tryLoadFile(sessionName, dataSet, numTopics))
			{
				// if loaded, set the LDAState to indicate that the initial
				// iterations are done
				theState.currentIterations = Constants.START_ITERATIONS;
			}
			else
			{
				// if not, we want to start a new LDA
				resetIters = true;
				resetConstraints = true;	
			}
		}
		else
		{
			theState = (LDAState)request.getSession().getAttribute("state");
		}
	
		
		// are we loading a session?
		if (loadingSession)
		{
			String sessionName = request.getParameter("loadSessionName");
			LDAState state = SessionFileUtils.loadLDAState(sessionName);
			theState = state;
			request.getSession(true).setAttribute("state",  theState);
		}
		
		// If we're NOT creating a new session, and the random seed still hasn't
		// been set, it's time to set it.
		if (!creatingSession && theState.randomSeed < 0)
		{
			theState.randomSeed = Math.abs(new Random().nextInt());
		}		
		
		//have we changed the label type? (ignore the form field if we're loading
		//an existing session, though)
		if (!loadingSession && request.getParameter("labelType") != null)
		{
			LabelType labelType = LabelType.valueOf(request
					.getParameter("labelType"));
			theState.labelType = labelType;
		}
		
		// reset the LDA state, if we've chosen the "reset" parameter
		if (resetIters)
		{
			theState.topicInfo = null;
			theState.currentIterations = 0;
			theState.newIterations = Constants.START_ITERATIONS;
			theState.append = false;
		}
		// add iterations, if we've decided to add them
		else if (continueIters)
		{
			theState.newIterations = Constants.STEP_ITERATIONS;
			theState.append = true;
		}
		else
		{
			theState.newIterations = 0;
		}
		
		// delete constraints, if we've chosen to do that
		if (resetConstraints)
		{
			theState.resetConstraints();
		}		

		// save the session, just in case
		SessionFileUtils.saveLDAState(theState);
		
		request.getRequestDispatcher("display.jsp").forward(request, response);
	}

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doGet(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException
	{
		process(request,response);
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doPost(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException
	{
		process(request,response);
	}

}
