package servlets;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import lda.external.LDACaller;
import lda.post.RawTopicInfo;
import lda.post.TopicInfoGetter;
import ontology.OntologyWriter;

import org.json.JSONObject;

import utils.Colors;
import utils.Constants;
import utils.DataSet;
import utils.LDAState;
import utils.LabelType;
import utils.DocumentInfo;
import utils.TopicInfo;
import utils.files.DefaultFileUtils;
import utils.files.SessionFileUtils;

/**
 * Servlet class that kicks off the LDA command-line program. When this command
 * is finished, the servlet populates a TopicInfo object from the results and
 * returns it in JSON form.
 */
public class RunLDAServlet extends HttpServlet
{
	private static final long serialVersionUID = 1L;

	/**
	 * @see HttpServlet#HttpServlet()
	 */
	public RunLDAServlet()
	{
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
			System.out.println("RUNLDASERVLET CALLED");
			request.getSession().setMaxInactiveInterval(3600);
			
			LDAState theState = (LDAState)request.getSession().getAttribute("state");
			
			DataSet dataSet = theState.dataSet;
			int numIterations = theState.newIterations;

			if (numIterations > 0)
			{
				// first create the constraint file
				OntologyWriter.createOntology(
						theState.constraints,
						dataSet.vocabFilename,
						SessionFileUtils.getConstraintFile(theState));

				int totalIterations = theState.currentIterations
						+ numIterations;

				LDACaller.callLDA(theState, totalIterations);
				System.out.println("DONE CALLING LDA");

			}

			
		} catch (Exception e)
		{
			e.printStackTrace();
		}
	}

}
