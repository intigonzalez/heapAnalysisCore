package org.heapexplorer.heapanalysis;

public class AnalysisType {
	public final String name;
	public final String description;
	public final int id;
	public 	AnalysisType(int id, String name, String description) {
		this.id = id;
		this.name = name;
		this.description = description;
	}
}
