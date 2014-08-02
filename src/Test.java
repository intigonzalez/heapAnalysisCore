import java.net.*;
import java.io.File;
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
				//obj = cl.newInstance();
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
					Thread.sleep(1000);
					String s = new String("Coco es un feo");
					System.out.printf("Iteration!!! %s %d\n", cl, f.getLong(null));
				}
				catch (Exception ex) {
				}
			}
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
	}
}
