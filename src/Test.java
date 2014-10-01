/*
 * #%L
 * org.heapexplorer.heapanalysis
 * $Id:$
 * $HeadURL:$
 * %%
 * Copyright (C) 2013 - 2014 org.kevoree
 * %%
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Lesser Public License for more details.
 * 
 * You should have received a copy of the GNU General Lesser Public 
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/lgpl-3.0.html>.
 * #L%
 */
import java.net.*;
import java.io.*;
import java.lang.reflect.*;
import java.util.*;

import org.heapexplorer.heapanalysis.*;
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
		STATS("stats", "Calculate the statistics of each principal", "stats ID_ANALYSIS"),
		SHOW("show", "Show the type of analysis already installed", ""),
		EXIT("exit", "Exit the application", "");
		
		String cmd;
		String description;
		String usage;

		Command(String cmd, String desc, String usage) {
			this.cmd = cmd;
			this.description = desc;		
			this.usage = usage;
		}
		
		boolean is(String s) {
			String[] stuff = s.split(" ");
			return stuff[0].equals(cmd);		
		}
		
		void printUsage() {
			System.out.printf("USAGE FOR %s: %s\n", cmd, usage);		
		}

		static String[] arguments(String s) {
			return s.split(" ");
		}
	}

	private static void showAnalysisType() {
		AnalysisType[] a = HeapAnalysis.getAnalysis();
		for (int i = 0 ; i < a.length ; i++) {
			System.out.printf("Analysis: %s\n\tID: %d\n\tDescription: ", a[i].name, a[i].id);  
			String s = 	a[i].description;
			int limit = 100;
			while (s.length() > limit) {
				System.out.println("\t" + s.substring(0, limit));	
				s = s.substring(limit);	
			}
			System.out.println("\t" + s);
		}	
	}

	private static void standardPrint(Object obj) {
		Object[] r = (Object[])obj;
		for (int j = 0 ; j < r.length ; j++) {
				
			ClassDetailsUsage[] details = (ClassDetailsUsage[])r[j];
			Arrays.sort(details, new Comparator() {
				public int compare(Object obj1, Object obj2) {
					ClassDetailsUsage a = (ClassDetailsUsage)obj1, b = (ClassDetailsUsage)obj2;
					if (a.totalSize > b.totalSize)
						return -1;
					else if (a.totalSize < b.totalSize)
						return 1;
					else if (a.nbObjects > b.nbObjects)
						return -1;
					else if (a.nbObjects < b.nbObjects)
						return 1;
					else
						return a.className.compareTo(b.className);
				}
				public boolean equals(Object obj) {
					return false;			
				}
			});
			int totalCount = 0;
			int totalSize = 0;
			for ( int i = 0 ; i < details.length ; i++ ) {
				totalCount += details[i].nbObjects;
				totalSize += details[i].totalSize;	
			}

			/* Print out sorted table */
			System.out.printf("Heap View, Total of %d objects found, with a total size of %d.\n\n",
		             totalCount, totalSize);

			System.out.printf("Nro.      Space      Count      Class Signature\n");		
			System.out.printf("--------- ---------- ---------- ----------------------\n");
			for (int i = 0 ; i < details.length ; i++) {
				if ( details[i].totalSize == 0 || i > 25 ) {
					break;
				}
				System.out.printf("%9d %10d %10d %s\n",
					i,
					details[i].totalSize, 
					details[i].nbObjects, 
					details[i].className);
			}
			System.out.printf("--------- ---------- ---------- ----------------------\n");	
		}
	}

	public static class NewObjectUpcallGetObjects implements UpcallGetObjects {

		public Object[] getJavaDefinedObjects(String id) {
			Object[] r = new Object[1000];
			System.out.println("Executing upcall");
			for (int k = 0; k < 1000 ; k++ )
				r[k] = new String(id);

			return r;
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

		// set callback
		HeapAnalysis.callback = new NewObjectUpcallGetObjects();

		// console
		System.out.printf("Started an running\n");
		BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
		System.out.printf("> ");
		String line = reader.readLine();
		while (line != null) {
			if (Command.STATS.is(line)) {
				String[] argsCmd = Command.arguments(line);
				if (argsCmd.length > 1 ) {
					Object r = HeapAnalysis.analysis(Integer.parseInt(argsCmd[1]));
					standardPrint(r);
				}
				else Command.STATS.printUsage();
			}
			else if (Command.SHOW.is(line)) {
				showAnalysisType();
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
