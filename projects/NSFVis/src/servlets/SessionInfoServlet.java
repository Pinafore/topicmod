package servlets;

import java.io.IOException;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import ontology.Constraint;

import org.json.JSONObject;

import utils.LDAState;
import utils.Utils;
import utils.files.SessionFileUtils;

/**
 * Servlet class that loads the saved state information for a given
 * user session. This servlet is called when the user chooses a session
 * in the drop-down menu on index.jsp, and the session information is
 * meant to be displayed on that page.
 */
public class SessionInfoServlet extends HttpServlet {
	private static final long serialVersionUID = 1L;
       
    /**
     * @see HttpServlet#HttpServlet()
     */
    public SessionInfoServlet() {
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
	
	private void process(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException
	{
		try
		{
			// Load the serialized LDAState object
			String sessionName = request.getParameter("sessionName");
			LDAState tempState = SessionFileUtils.loadLDAState(sessionName);
			
			JSONObject obj = new JSONObject();
			obj.put("sessionName", sessionName);
			obj.put("numIterations", tempState.currentIterations);
			obj.put("dataSetName", tempState.dataSet.name);
			obj.put("numTopics", tempState.numTopics);
			obj.put("labelType", tempState.labelType.getDisplayName());
			obj.put("constraints", constraintsToString(tempState));
			
			response.getWriter().write(obj.toString());
			response.setContentType("application/json");
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		
	}
	
	// Convert the constraint list to a single human-readable string,
	// to be displayed on a web page.
	private String constraintsToString(LDAState tempState)
	{
		String str = "";
		for (Constraint constraint : tempState.constraints)
		{
			if (str.length() > 0)
				str += "; ";
			
			if (constraint.isCannotLink)
				str += "(Cannot Link) ";
			else
				str += "(Must Link) ";
			
			str += Utils.listToString(constraint.words);
		}
		
		if (str.length() == 0)
			str = "(None)";
		
		return str;
	}

}
