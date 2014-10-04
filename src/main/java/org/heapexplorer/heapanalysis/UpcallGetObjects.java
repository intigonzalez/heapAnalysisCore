package org.heapexplorer.heapanalysis;

public interface UpcallGetObjects
{
	public Object[] getJavaDefinedObjects(String ids);

    public boolean mustAnalyse(String ids);
}
