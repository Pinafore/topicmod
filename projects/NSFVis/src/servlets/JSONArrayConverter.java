package servlets;

import org.json.JSONArray;

/**
 * Methods for converting from a Java array to a JSON array, overloaded
 * for various data types.
 * 
 *  */
public class JSONArrayConverter
{
    public static<T> JSONArray toArray(T[] items)
    {	
    	JSONArray array = new JSONArray();
    	for (T item : items)
    	{
    		array.put(item);
    	}
    	return array;
    }
    
    public static JSONArray toArray(double[] items)
    {
    	JSONArray array = new JSONArray();
    	for (double item : items)
    	{
    		array.put(item);
    	}
    	return array;
    }    
    
    public static JSONArray toArray(int[] items)
    {
    	JSONArray array = new JSONArray();
    	for (int item : items)
    	{
    		array.put(item);
    	}
    	return array;
    }      
    
    public static JSONArray to2DArray(int[][] items)
    {
    	JSONArray array = new JSONArray();
    	for (int[] item : items)
    	{
    		array.put(toArray(item));
    	}
    	return array;
    }
    
    public static JSONArray to2DArray(double[][] items)
    {
    	JSONArray array = new JSONArray();
    	for (double[] item : items)
    	{
    		array.put(toArray(item));
    	}
    	return array;
    }    
}
