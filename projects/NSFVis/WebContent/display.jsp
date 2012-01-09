<%@ page language="java" contentType="text/html; charset=ISO-8859-1"
    pageEncoding="ISO-8859-1"%>
<%@ page import="utils.*" %>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">

<html>
  <head>
    <title>Visualization</title>
    <link type="text/css" rel="stylesheet" href="ex.css"/>
    <link type="text/css" rel="stylesheet" href="visual.css"/>
    <link type="text/css" rel="stylesheet" href="tables.css"/>    
    <script type="text/javascript" src="js/libs/protovis-d3.2.js"></script>
	<script type="text/javascript" src="js/libs/jquery-1.4.2.js"></script>
 	<script type="text/javascript" src="js/libs/simplemodal.js"></script>   	   	    
    <script type="text/javascript" src="js/common.js"></script>
    <script type="text/javascript" src="js/colors.js"></script> 
    <script type="text/javascript" src="js/document.js"></script>   
    <script type="text/javascript" src="js/stacked.js"></script>
    <script type="text/javascript" src="js/grid.js"></script>    
    <script type="text/javascript" src="js/legend.js"></script>
    <script type="text/javascript" src="js/visPage.js"></script>              


  </head>
  <body>
  
  <div id = "leftBar">
  	<div class ="barLink"><a href="index.jsp">Select Dataset</a></div>
  	<div class ="barLink">Visualization</div>
  	<div class ="barLink"><a href="constraints.jsp">Select Constraints</a></div>
  </div>

  <div id="rightBar">
  	<div class="barLink">
  		<form action="InitServlet" method="post">
  			Change Label Type:
  			<select id="labelType" name="labelType">
				<%
					LDAState theState = (LDAState)request.getSession().getAttribute("state");
					for (LabelType type : LabelType.values())
					{
						out.println("<option value='" + type.toString() + "'");
						if (type == theState.labelType)
						{
							out.println(" selected");
						}
						
						out.println(">" + type.getDisplayName()+ "</option>");
					}
				%>
				
			</select> &nbsp;
			<input type="submit" value="Change"/>
  		</form>
  	</div>
    <div class="barLink"><a href="InitServlet?reset=true">Reset Topics</a></div>
  	<div class="barLink"><a href="InitServlet?continue=true">Refine Topics (25 iterations)</a></div>
  </div>

  <div id="mainHeadline">Data Visualization</div>
  
  <div id="centerBar">
  	<div class="centerBarLink"><a href="javascript:selectGrid()">Grid View</a></div>
  	<div class="centerBarLink"><a href="javascript:selectStacked()">Area Chart View</a></div>
  </div>

  <div id="outerVis">
  
  <div id="innerVis">
  
  	<div id="areaTD">
		<div id="stacked">
			<div class="headline">Area Chart</div>
			<div id="stackedVis"></div>
  		</div>
  			
  		<div id="grid">
  			<div class="headline">Grid Chart</div>
			<% out.println(TableCreator.getGrid(theState.numTopics, theState.dataSet.years)); %>
  		</div>
  	</div>
	
	<div id="selectTD">
		<div class="headline">Legend</div>
		<% out.println(TableCreator.getLegend(theState.numTopics, theState.labelType)); %>
			<a href="javascript:selectAllTopics()">Show All Topics</a><br/>
			<a href="javascript:clearAllTopics()">Hide All Topics</a>		
	</div>
  
  </div>
  
  <div id="docTD">
    	<div class="headline">Topic Words by Year</div>
    	<div id="yearLabels"></div>
  
  		<div class="headline">Most Relevant Documents</div>
		<div id="documentText" class="headline"></div>
		<div id="documentVis"></div>
  </div> 

  </div>

	<script type="text/javascript+protovis">

	// this function is part of page.js
	$(document).ready(doAjax);
	
	</script>
  
  </body>
</html>


