import java.net.*;
import java.io.*;
import java.lang.reflect.*;
import java.security.Permission;
/**
 *
*/
public class Test2 {

	private static class ExitTrappedException extends SecurityException { }

	private static void forbidSystemExitCall() {
		final SecurityManager securityManager = new SecurityManager() {
			public void checkPermission( Permission permission ) {
				if( permission.getName().startsWith("exitVM") ) {
				  throw new ExitTrappedException() ;
				}
			}
		} ;
		System.setSecurityManager( securityManager ) ;
	}

	private static void enableSystemExitCall() {
		System.setSecurityManager( null ) ;
	}

	public static class ExecutingDacapo extends Thread {
		int id;
		public ExecutingDacapo(int id) {			
			super(new ThreadGroup("another guy " + id), "aaaaa");	
			this.id = id;	
		}
		
		public void run() {
			Class<?> cl = null;
			Object obj = null;
			try {
				ClassLoader loader = getContextClassLoader();
				cl = loader.loadClass("Harness");
                Method method = cl.getMethod("main", new Class[]{String[].class});

                method.invoke(null,new Object[]{new String[]{
                        "--no-validation",
                        "-n",
                        "3",
						"--scratch-directory",
						"scratch"+id,
                        "avrora"}});
			}
			catch (InvocationTargetException ex) {
					if (!ex.getCause().getClass().equals(ExitTrappedException.class))
						ex.printStackTrace();
			}
			catch(ExitTrappedException ex) {
				ex.printStackTrace();
			}
			catch (Exception ex) { ex.printStackTrace();	}
		}	
	}

	public static class DoingAnalysis extends Thread {
		private boolean stopped = false;
		public void run() {
			while(!isStopped1()) {
				try {
					Thread.sleep(1000);
					System.err.println("BEFORE");
					System.err.flush();
					HeapAnalysis.analysis(HeapAnalysis.PER_THREAD_GROUP);
					System.err.println("AFTER");
					System.err.flush();
				}
				catch (Exception ex) { 
					ex.printStackTrace();
				}
			}
		}

		private synchronized boolean isStopped1() {
			return stopped;
		}
		
		public synchronized void stop1() {
			stopped = true;
		} 
	}

	public static void theOtherGuy(int[] ids) {
		try {
			ExecutingDacapo[]  ths = new ExecutingDacapo[ids.length];
			for (int i = 0 ; i < ths.length ; i++) {
				ExecutingDacapo th = new ExecutingDacapo(ids[i]);
				th.setContextClassLoader(new URLClassLoader(new URL[]{new File("/home/inti/Downloads/dacapo-9.12-bach.jar").toURI().toURL()}));
				ths[i] = th;				
				th.start();
			}
			for (int i = 0 ; i < ths.length ; i++) {
				ths[i].join();
			}
		}
		catch (Exception ex) {}
	}

	public static void main(String[] args) throws Exception {
		forbidSystemExitCall();
		boolean[] bb = new boolean[1000000];
		

		DoingAnalysis a = new DoingAnalysis();
		a.start();
		theOtherGuy(new int[] {1, 2, 3, 4});

		int i = 1200;
		/*while(i > 0) {
				try {
					Thread.sleep(100);
					System.gc();
					i--;
				}
				catch (Exception ex) { ex.printStackTrace(); }
			}
*/
		
		//a.stop1();	
		a.join();
	}
}
