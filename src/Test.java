import java.net.*;
import java.io.*;
import java.lang.reflect.Field;
/**
 *
*/
public class Test {
	public static class Thread1 extends Thread {
		public long  t = 567;
		public Object another = null;
		public long[] numbers = new long[1000];
		public void run() {
			Class<?> cl = null;
			Field f = null;
			Object obj = null;
			try {
				ClassLoader loader = getContextClassLoader();
				cl =loader.loadClass("Another");
				if (cl != null)
					System.out.println("The class was loaded");
				f = cl.getField("pepe");
				obj = cl.newInstance();
				f.setLong(obj, t);
			}
			catch (Exception ex) {
				ex.printStackTrace();			
			}
			Object[] objs = new Object[100];
			String sss = new String("It seems to be working!!!!!!!!!!!!");
			for (int i = 0 ; i < objs.length ; i++)
				objs[i] = new Integer(i);
			while(true) {
				try {
					Thread.sleep(60000);
					String s = new String("Coco es un feo");
					System.out.printf("Iteration!!! %s %d\n", cl, f.getLong(null));
				}
				catch (Exception ex) {
				}
			}
		}
	}

	private static final String ANALYSIS_COMMAND = "stats";

	private static enum Command {
		STATS("stats", "Calculate the statistics of each principal"),
		EXIT("exit", "Exit the application");
		
		String cmd;
		String description;

		Command(String cmd, String desc) {
			this.cmd = cmd;
			this.description = desc;		
		}
		
		boolean is(String s) {
			return s.equals(cmd);		
		}
	}

	public static void main(String[] args) throws Exception {
		boolean[] bb = new boolean[1000000];
		Thread1 th = new Thread1();
		URLClassLoader loader = new URLClassLoader(new URL[]{new File("Th1/").toURI().toURL()});
		th.t = 111111111;		
		th.another = bb;
		th.setContextClassLoader(loader);
		th.setName("The guy");
		th.start();
		Thread1 th2 = new Thread1();
		URLClassLoader loader2 = new URLClassLoader(new URL[]{new File("Th1/").toURI().toURL()});
		th2.t = 9999;
		th2.another = bb;
		th2.setContextClassLoader(loader2);		
		th2.setName("The guy");
		th2.start();
		System.out.printf("Started an running\n");
		BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
		System.out.printf("> ");
		String line = reader.readLine();
		while (line != null) {
			if (Command.STATS.is(line)) {
				HeapAnalysis.analysis();
			}
			else if(Command.EXIT.is(line))
				System.exit(0);
			else {
				System.out.printf("Usage:\n");
				for (Command c: Command.values())
					System.out.printf("\t%s - Description\n", c.cmd, c.description);
			}
			System.out.printf("> ");
			line = reader.readLine();
		}
	}
}
