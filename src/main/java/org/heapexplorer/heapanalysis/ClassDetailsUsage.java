package org.heapexplorer.heapanalysis;

public class ClassDetailsUsage {
	public String className;
	public int nbObjects;
	public int totalSize;
	public ClassDetailsUsage(String name, int nbObjs, int size) {
		className = name;
		nbObjects = nbObjs;
		totalSize = size;
	}

    @Override
	public String toString() {
		return String.format("This is shit %s %d %d", className, nbObjects, totalSize);
	}
}
