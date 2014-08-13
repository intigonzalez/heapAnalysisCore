public class HeapAnalysis {
	public static native void analysis(int id);

	private static native int count_of_analysis() ;
	private static native AnalysisType getAnalysis(int index);

	public static AnalysisType[] getAnalysis() {
		int n = count_of_analysis();
		AnalysisType[] a = new AnalysisType[n];
		for (int i = 0 ; i < n ; i++) {
			a[i] = (AnalysisType)getAnalysis(i);	
		}
		return a;
	} 
}
