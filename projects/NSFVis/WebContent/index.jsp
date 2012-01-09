<%@ page language="java" contentType="text/html; charset=ISO-8859-1"
    pageEncoding="ISO-8859-1"%>
<%@ page import="utils.*" %>
<%@ page import="utils.files.*" %>
<%@ page import="java.util.*" %>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">

<%
	LDAState theState = (LDAState)request.getSession().getAttribute("state");
%>

<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
	<title>Select Data Set</title>
 
	<link type="text/css" rel="stylesheet" href="ex.css"/>
	<link type="text/css" rel="stylesheet" href="visual.css"/>
	<script type="text/javascript" src="js/libs/jquery-1.4.2.js"></script>
	<script type="text/javascript" src="js/sessions.js"></script>

</head>
<body>

<% if (theState != null && theState.dataSet != null) { %>
  <div id = "leftBar">
  	<div class ="barLink">Select Dataset</div>
  	<div class ="barLink"><a href="display.jsp">Visualization</a></div>
  	<div class ="barLink"><a href="constraints.jsp">Select Constraints</a></div>
  </div>
<% } %>

  <div id="mainHeadline">Select Dataset</div>

	<div id="selectMain">
	<div id="selectInner">
	<form action="InitServlet" method="POST">
	
	
		<div id="selectExisting">
	
			<div class="selectHeader">
				<% List<String> sessionNames = SessionFileUtils.getSessionNames(); %>
			
				<input type="radio" name="sessionType" value="load"
					<% if (sessionNames.size() == 0) out.println("disabled=disabled"); %>
				/> 
				Load Existing Session	
			</div>

			<div class="selectLine">
				<div class="selectCell">
					Session Name
				</div> 
				<div class="selectCell">
					<select id="loadSessionName" name="loadSessionName"
						<% if (sessionNames.size() == 0) out.println("disabled=disabled"); %>
					>
							<%
								for (String name : sessionNames)
								{
									out.println("<option value='" + name
											+ "'>" + name + "</option>");
								}
							%>
					</select> 
				</div>
			</div>
			
			<div id="sessionDisplay">
				<div id="mainDisplay"></div>
				<div id="constraintDisplay"></div>
			</div>
			
		</div>
		
		<div id="selectNew">
			<div class="selectHeader">
				<input type="radio" name="sessionType" value="new" checked/> Create New Session
			</div>
		
			<div class="selectLine">
				<div class="selectCell">Session Name</div>
				<div class="selectCell">
					<input type="text" id="newSessionName" name="newSessionName">
				</div>
			</div>
		
			<div class="selectLine">
				<div class="selectCell">Number of Topics</div>
				<div class="selectCell">
					<input type="text" id="numTopics" name="numTopics">
				</div>
			</div>
		
			<div class="selectLine">
				<div class="selectCell">Label Type</div>
				<div class="selectCell">
					<select id="labelType" name="labelType">
					<%
						for (LabelType type : LabelType.values())
						{
							out.println("<option value='" + type.toString()
									+ "'>" + type.getDisplayName() + "</option>");
						}
					%>
					
					</select>
				</div>
			</div>
			
			<div class="selectLine">
				<div class="selectCell">Data Set</div>
				<div class="selectCell">
					<select id="dataSet" name="dataSet">
					<%
						for (int i = 0; i < DataSet.getDataSets().size(); i++)
						{
							DataSet dataSet = DataSet.getDataSets().get(i);
							out.println("<option value='" + i
									+ "'>" + dataSet.name + "</option>"); 
						}
					%>
					</select>
				</div>
			</div>

		</div>
		
		<input type="submit" value="Submit"/>
		
	</form>
	</div>
	</div>
</body>

	<script type="text/javascript">

		$(document).ready(doAjax);
		$('#loadSessionName').change(doAjax);
		
	</script>

</html>