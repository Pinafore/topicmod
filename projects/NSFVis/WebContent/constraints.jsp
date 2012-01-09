<%@ page language="java" contentType="text/html; charset=ISO-8859-1"
    pageEncoding="ISO-8859-1"%>
<%@ page import="utils.*" %>
<%@ page import="utils.files.*" %>
<%@ page import="ontology.*" %>
<%@ page import="java.util.*" %>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">

<%
	LDAState theState = (LDAState)request.getSession().getAttribute("state");
	boolean constraintChanged = false;

	// did we add or edit a constraint?
	String constraintWords = "";
	if (request.getParameter("constraintWords") != null)
		constraintWords = request.getParameter("constraintWords");
		
	if (constraintWords.length() > 0)
	{
		System.out.println("Words: " + constraintWords);
		String type = request.getParameter("type");
		boolean isCannotLink = (type.equals("cannotLink"));
		
		int editIndex = -1;
		if (request.getParameter("endEditIndex") != null)
		{
			editIndex = Integer.parseInt(request.getParameter("endEditIndex"));
		}
		
		if (editIndex < 0)
		{
			theState.addConstraint(Utils.stringToList(constraintWords), isCannotLink);
		}
		else
		{
			theState.replaceConstraint(editIndex, Utils.stringToList(constraintWords), isCannotLink);
		}
		constraintChanged = true;
	}
			
	// did we delete a constraint?
	int deleteIndex = -1;
	if (request.getParameter("deleteIndex") != null)
		deleteIndex = Integer.parseInt(request.getParameter("deleteIndex"));
	
	if (deleteIndex >= 0)
	{
		theState.deleteConstraint(deleteIndex);
		constraintChanged = true;
	}
	
	// initialize some values to be displayed in the page
	String dataSetName = theState.dataSet.name;
	List<Constraint> constraints = theState.constraints;
	
	// are we starting to edit a constraint?
	int currEditIndex = -1;
	if (request.getParameter("startEditIndex") != null)
		currEditIndex = Integer.parseInt(request.getParameter("startEditIndex"));
	String currConstraintWords = Constraint.getJavascript(constraints, currEditIndex);
	
	// initialize some more stuff
	Set<String> allWords = Constraint.getAllWords(constraints, currEditIndex);	
	String[][] topicWords = theState.topicInfo.getTopicWords();
	int numTopics = theState.topicInfo.getNumTopics();
	
	if (constraintChanged)
	{
		
		// create the constraint file
		OntologyWriter.createOntology(
				theState.constraints,
				theState.dataSet.vocabFilename,
				SessionFileUtils.getConstraintFile(theState));
	}
%>

<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<title>Select Constraints</title>
<link type="text/css" rel="stylesheet" href="ex.css"/>
<link type="text/css" rel="stylesheet" href="visual.css"/>
<script type="text/javascript" src="js/libs/jquery-1.4.2.js"></script>
<script type="text/javascript" src="js/constraints.js"></script>
<script type="text/javascript">

	function deleteConstraint(id)
	{
		$('#deleteIndex').attr('value', id);
		$('#myform').submit();
	}

	function editConstraint(id)
	{
		$('#startEditIndex').attr('value', id);
		$('#myform').submit();
	}	
	
	function addConstraint()
	{	
		var str = getConstraintString();
		$("#constraintWords").attr('value', str);
		$('#myform').submit();
	}

	// this calls code in constraints.js
	function ready()
	{
		setConstraintWords(<%= currConstraintWords %>);
		var html = getConstraintHTML();
		$('#existing').html(html);
	}

	function addMe(word)
	{
		addConstraintWord(word);
		changeColor(word, true);
		var html = getConstraintHTML();
		$('#existing').html(html);
	}

	function deleteMe(word)
	{
		deleteConstraintWord(word);
		changeColor(word, false);
		var html = getConstraintHTML();
		$('#existing').html(html);		
	}

	function warn()
	{
		var conf = confirm('You have made changes to the constraints. Return to the Visualization page without updating the topics?');
		if (conf)
			location.href = "display.jsp";				
	}

	$(document).ready(ready);

</script>

</head>
<body>

  <div id = "leftBar">
  	<div class ="barLink"><a href="index.jsp">Select Dataset</a></div>
  	<div class ="barLink">
  	
  	<%
  		if (theState.dirty)
  		{
  			out.println("<a href='javascript:warn()'>Visualization</a>");
  		}
  		else
  		{
  			out.println("<a href='display.jsp'>Visualization</a>");
  		}
  	
  	%>
  	
  	</div>
  	<div class ="barLink">Select Constraints</div>
  </div>
  
  <div id="rightBar">
    <div class="barLink"><a href="InitServlet?reset=true">Reset Topics</a></div>
  	<div class="barLink"><a href="InitServlet?continue=true">Refine Topics (25 iterations)</a></div>
  </div>
  
  <div id="mainHeadline">Select Constraints</div>
  <div class="headline">Data set name: <%= dataSetName %></div>
  
  	<form action="constraints.jsp" id="myform" name="myform" method="post">
  	<input type="hidden" name="deleteIndex" id="deleteIndex" value="-1"/>
  	<input type="hidden" name="startEditIndex" id="startEditIndex" value="-1"/>
	<input type="hidden" name="endEditIndex" id="endEditIndex" value="<%=currEditIndex %>"/>
  	<input type="hidden" name="constraintWords" id="constraintWords" value=""/>

	<div id="newConstraints">
  		<div id="newConstraintsLeft">
	  		<div class="instructions"> Select words: </div> 
	  		<div id="scrollArea">
	  		<%
	  			for (int i = 0; i < numTopics; i++)
	  			{
	  				out.println("<div class='selectTopic'>");
	  				out.println("<div class='selectTopicName'>");
	  				out.println("Topic " + i + "</div>");
	  				
	  				out.println("<div class='selectTopicWords'>");
	  				for (String word : topicWords[i])
	  				{
	  					String cssClass = allWords.contains(word) ? "topicWordExisting" : "topicWord";
	  					
	  					String command = "javascript:addMe('" + word + "')";
	  					String link = "<a class='" + cssClass + "' href=\"" + command + "\"" +
	  						" name = '" + word + "'>" + word + "</a>";
	  					out.println(link);
	  			
	  				}
	  				out.println("</div></div>");
	  			}
	  		
	  		%>
	  		</div>
		</div>	
		<div id="newConstraintsRight">
		  	<div class="instructions"> Currently selected words (click to deselect):</div> 
  			<div id="existing">
  			</div>
  			<div id="constraintButtons">
			  	<div class="constraintButton">
			  		<input type="radio" name="type" value="mustLink" checked/> Must Link
			  	</div>
			  	<div class="constraintButton">
	  				<input type="radio" name="type" value="cannotLink"/> Cannot Link
	  			</div>
	  			<div class="constraintButton">
	  				<% String buttonName = (currEditIndex >= 0) ? "Update" : "Add"; %>
	  				<input type="button" name="newConstraint" value="<%= buttonName %>" onClick="addConstraint();"/>
	  				<input type="submit" name="cancel" value="Cancel"/>
	  			</div>
			</div>
			
			<div id="currentConstraints">
			  	<div class="headline">Current constraints:</div>	
			  	<div id="currentConstraintsInner">
					<%
						for (int i = 0; i < constraints.size(); i++)
						{
							String cssClass = (i == currEditIndex) ? "editConstraintRow" : "currentConstraintRow";							
							out.println("<div class='" + cssClass + "'>");
							
							out.println("<div class='deleteCell'>");
							out.println("<a href='javascript:deleteConstraint(" + i + ");'>" +
									"<img src='images/delete.jpg'/></a>");
							out.println("</div><div class='deleteCell'>");							
							out.println("<a href='javascript:editConstraint(" + i + ");'>" +
							"<img src='images/edit.png'/></a>");
							out.println("</div><div class='currentConstraintCell'>");
						 	if (constraints.get(i).isCannotLink)
						 		out.println("(Cannot Link) ");
						 	else
						 		out.println("(Must Link) ");
						 	out.println(Utils.listToString(constraints.get(i).words));	
						 	out.println("</div></div>");
						}
						
						if (constraints.size() == 0)
						{
							out.println("<div class='currentConstraintRow'>(None)</div>");
						}
					
					%>	
			
			  	</div>
			 </div>
		</div>
  	</div>
  
	</form>
	
</body>
</html>