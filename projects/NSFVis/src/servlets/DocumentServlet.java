package servlets;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.json.JSONArray;
import org.json.JSONObject;

import utils.Constants;
import utils.DataSet;
import utils.LDAState;
import utils.DocumentInfo;

/**
 * Servlet that returns information (in JSON form) about a particular
 * set of documents.
 */
public class DocumentServlet extends HttpServlet
{
	private static final long serialVersionUID = 1L;

	/**
	 * Default constructor.
	 */
	public DocumentServlet()
	{
		// TODO Auto-generated constructor stub
	}

	/**
	 * Creates a JSON object from information about a document.
	 */
	private JSONObject createObject(String link, String title, boolean hasLink, double[] props)
	{
		JSONObject obj = new JSONObject();
		obj.put("link", link);
		obj.put("hasLink", hasLink);
		obj.put("title", title);
		obj.put("props", JSONArrayConverter.toArray(props));
		return obj;
		
	}
	
	private void process(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException
	{
		LDAState theState = (LDAState)request.getSession().getAttribute("state");
		
		List<DocumentInfo> docInfo = new ArrayList<DocumentInfo>();
		int topic = Integer.parseInt(request.getParameter("topic"));
		
		if (request.getParameter("year") != null)
		{
			// get the documents for a single topic and year
			int year = Integer.parseInt(request.getParameter("year"));	
			docInfo = theState.topicInfo.getDocumentsByYear(year, topic);
		}
		else
		{
			// get all the documents for a given topic
			docInfo = theState.topicInfo.getAllDocuments(topic);
		}
		
		// truncate if necessary
		if (docInfo.size() > Constants.DOCUMENTS_TO_DISPLAY)
		{
			docInfo = docInfo.subList(0, Constants.DOCUMENTS_TO_DISPLAY);
		}
		
		// Convert to JSON form
		JSONArray array = new JSONArray();
		for (DocumentInfo info : docInfo)
		{
			String title = DataSet.getDisplayTitle(theState.dataSet, info.title);
			String link = DataSet.getLink(theState.dataSet, info.title);
			boolean hasLink = (link != null);
			
			JSONObject obj = createObject("" + link, title, hasLink, 
					info.topicProps);
			array.put(obj);
		}
		
		JSONArray labelArray = new JSONArray();
		String[][] yearLabels = theState.topicInfo.getYearLabels(theState.labelType);
		for (int i = 0; i < theState.dataSet.years.size(); i++)
		{
			String labelStr = yearLabels[i][topic];
			labelArray.put(labelStr);
		}
		
		JSONObject outerObj = new JSONObject();
		outerObj.put("documents", array);
		outerObj.put("labels", labelArray);
		
		System.out.println(array.toString());
		response.getWriter().write(outerObj.toString());
		response.setContentType("application/json");

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

}
