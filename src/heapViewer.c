/*
 * Copyright (c) 2004, 2006, Oracle and/or its affiliates. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *   - Neither the name of Oracle nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This source code is provided to illustrate the usage of a given feature
 * or technique and has been deliberately simplified. Additional steps
 * required for a production-quality application, such as security checks,
 * input validation and proper error handling, might not be present in
 * this sample code.
 */


#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jni.h"
#include "jvmti.h"

#include "agent_util.h"
#include "ResourceManagement.h"

#include "thread_principal.h"
#include "jvm_principal.h"
#include "whole_heap_principal.h"

/* Global static data */
typedef struct {
	jvmtiEnv      *jvmti;
    jboolean      vmDeathCalled;
    jboolean      dumpInProgress;
    jrawMonitorID lock;
    ClassInfo* info_classes;
} GlobalData;
static GlobalData globalData, *gdata = &globalData;


/* Enter agent monitor protected section */
static void
enterAgentMonitor(jvmtiEnv *jvmti)
{
    jvmtiError err;

    err = (*jvmti)->RawMonitorEnter(jvmti, gdata->lock);
    check_jvmti_error(jvmti, err, "raw monitor enter");
}

/* Exit agent monitor protected section */
static void
exitAgentMonitor(jvmtiEnv *jvmti)
{
    jvmtiError err;

    err = (*jvmti)->RawMonitorExit(jvmti, gdata->lock);
    check_jvmti_error(jvmti, err, "raw monitor exit");
}

/* Compare two ClassDetails */
static int
compareDetails(const void *p1, const void *p2)
{
    return ((ClassDetails*)p2)->space - ((ClassDetails*)p1)->space;
}

static void
printTable(ClassDetails* details, int count)
{
	int i;
	int totalCount = 0;
	int totalSize = 0;
	for ( i = 0 ; i < count ; i++ ) {
		totalCount += details[i].count;
		totalSize += details[i].space;	
	}

	/* Sort details by space used */
	qsort(details, count, sizeof(ClassDetails), &compareDetails);

	/* Print out sorted table */
	stdout_message("Heap View, Total of %d objects found, with a total size of %d.\n\n",
		             totalCount, totalSize);

	stdout_message("Nro.      Space      Count      Class Signature\n");
	stdout_message("--------- ---------- ---------- ----------------------\n");

	for ( i = 0 ; i < count ; i++ ) {
		if ( details[i].space == 0 || i > 25 ) {
		    break;
		}
		stdout_message("%9d %10d %10d %s\n",
			i,
		    details[i].space, 
			details[i].count, 
			getClassSignature(&details[i]));
	}
	stdout_message("--------- ---------- ---------- ----------------------\n\n");
}


/** 
 * Explore the memory consumption of each principal 
 * (follows different strategies) 
 */
static 
void explorePrincipals(
		jvmtiEnv* jvmti, 
		int count_classes,
		jclass* classes,
		CreatePrincipals createPrincipals)
{
    int j;
    int i;
	int count_principals;
	jlong tmp;
    ResourcePrincipal* principals;
    jvmtiError err;
	ClassDetails* details;
	ResourcePrincipal shared_stuff_principal;

	struct timeval tv;
	struct timeval start_tv;
	double elapsed = 0.0;

	gettimeofday(&start_tv, NULL);

    /** General algorithm
     * 1 - Build the collection RPs of resource principals
     * 2 - Exclude objects and classes that are shared among principals
     * 3 - For each principal R in RPs do
     * 		3.1 - Initialize classes' information
     * 		3.2 - Calculate roots
     * 		3.3 - Explore from the roots
     * 		3.4 - Print results
     * 		3.5 - Clean-up
     * 4 - Free collection RPs
     * */

    // obtain data for the whole JVM
    // Step 1
	/* Allocate space to hold the details */
	count_principals = createPrincipals(jvmti, &principals, gdata->info_classes , count_classes);
    // Step 2 (Empty if the resource principal is the whole JVM)
	
    // Step 3

	for ( i = 0 ; i < count_classes ; i++ ) {
           	/* Tag this jclass */
           	err = (*jvmti)->SetTag(jvmti, classes[i],
                          tagForObject(NULL));
           	check_jvmti_error(jvmti, err, "set object tag");
   	}
    for (j = 0 ; j < count_principals ; ++j) {
    	// step 3.1	
       	for ( i = 0 ; i < count_classes ; i++ ) {
				jlong t_ptr;
               	/* Tag this jclass */
               	err = (*jvmti)->GetTag(jvmti, classes[i], &t_ptr);
               	check_jvmti_error(jvmti, err, "set object tag");
				setUserDataForTag(t_ptr, &(principals[j].details[i]));
       	}

		// step 3.2 (Empty because all the roots are valid)
		// step 3.3
		/* Iterate through the heap and count up uses of jclass */
		((LocalExploration)(principals[j].strategy_to_explore))(jvmti,  &principals[j]);
		// step 3.4 
		printTable(principals[j].details, count_classes);
		// step 3.5
	   	for ( i = 0 ; i < count_classes ; i++ ) {
	       	jlong t_ptr;
           	/* Tag this jclass */
           	err = (*jvmti)->GetTag(jvmti, classes[i], &t_ptr);
           	check_jvmti_error(jvmti, err, "set object tag");
			setUserDataForTag(t_ptr, NULL);
	   	}
    }
    // step 4
    for (j = 0 ; j < count_principals ; ++j)
    	free(principals[j].details);
    free(principals);
	removeTags(jvmti);
	
	gettimeofday(&tv, NULL);
	elapsed = (tv.tv_sec - start_tv.tv_sec) * 1000000.0 +
  		(tv.tv_usec - start_tv.tv_usec);
	stdout_message("\n=========================================\n Elapsed time: %lf microseconds\n=========================================\n", elapsed);
}

/* Callback for JVMTI_EVENT_DATA_DUMP_REQUEST (Ctrl-\ or at exit) */
static void JNICALL
dataDumpRequest(jvmtiEnv *jvmti)
{
    enterAgentMonitor(jvmti); {
        if (!gdata->vmDeathCalled && !gdata->dumpInProgress ) {
            jvmtiError         err;
            jclass            *classes;
            jint               totalCount;
            jint               count;
            jint               i;
	    	jint			   j;
			jint               count_principals;
			ResourcePrincipal* principals;

		    gdata->dumpInProgress = JNI_TRUE;


		    /* Get all the loaded classes */
		    err = (*jvmti)->GetLoadedClasses(jvmti, &count, &classes);
		    check_jvmti_error(jvmti, err, "get loaded classes");

			/* Setup space to hold signature of classes*/
			gdata->info_classes = (ClassInfo*)calloc(sizeof(ClassInfo), count);
			if (gdata->info_classes == NULL)
				fatal_error("ERROR: Ran out of malloc space\n");

			for ( i = 0 ; i < count ; i++ ) {
				char *sig;
				/* Get and save the class signature */
				err = (*jvmti)->GetClassSignature(jvmti, classes[i], &sig, NULL);
				check_jvmti_error(jvmti, err, "get class signature");
				if ( sig == NULL )
					fatal_error("ERROR: No class signature found\n");
				gdata->info_classes[i].signature = strdup(sig);
				gdata->info_classes[i].is_clazz_clazz = !strcmp(sig, "Ljava/lang/Class;");
				deallocate(jvmti, sig);
			}

			// initial version
			explorePrincipals(jvmti, // the environment  
					count, // number of loaded classes 
					classes, // loaded classes
					&create_single_principal // strategy to create principals
					); 
		   
			// version Inti
			stdout_message("Starting to explore all alive references\n");
			explorePrincipals(jvmti,
					count,
					classes,
					&createPrincipal_WholeJVM // strategy to create principals
					);

			// Version2 : Inti
			explorePrincipals(jvmti,
					count,
					classes,
					&createPrincipal_per_thread // strategy to create principals
					);

		    /* Free up all allocated space */
		    deallocate(jvmti, classes);
		    for ( i = 0 ; i < count ; i++ )
		        if ( gdata->info_classes[i].signature != NULL )
		            free(gdata->info_classes[i].signature);

			free(gdata->info_classes);
			gdata->info_classes = NULL;

            gdata->dumpInProgress = JNI_FALSE;
        }
    } exitAgentMonitor(jvmti);
}

/* Java Native Method for entry */
static void
HEAPANALYSIS_native_analysis(JNIEnv *env, jclass klass)
{
	dataDumpRequest(gdata->jvmti);
}

/* Callback for JVMTI_EVENT_VM_START */
static void JNICALL
cbVMStart(jvmtiEnv *jvmti, JNIEnv *env)
{
    enterAgentMonitor(jvmti); {
        jclass   klass;
        jfieldID field;
        int      rc;

        /* Java Native Methods for class */
        static JNINativeMethod registry[1] = {
            {"analysis", "()V",
                (void*)&HEAPANALYSIS_native_analysis}
        };

        /* The VM has started. */
        stdout_message("VMStart\n");

        /* Register Natives for class whose methods we use */
        klass = (*env)->FindClass(env, "HeapAnalysis");
        if ( klass == NULL ) {
            fatal_error("ERROR: JNI: Cannot find %s with FindClass\n",
                        "HeapAnalysis");
        }
        rc = (*env)->RegisterNatives(env, klass, registry, 1);
        if ( rc != 0 ) {
            fatal_error("ERROR: JNI: Cannot register native methods for %s\n",
                        "HeapAnalysis");
        }

    } exitAgentMonitor(jvmti);
}

/* Callback for JVMTI_EVENT_VM_INIT */
static void JNICALL
vmInit(jvmtiEnv *jvmti, JNIEnv *env, jthread thread)
{
    /*
	enterAgentMonitor(jvmti); {
        jvmtiError          err;

        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                            JVMTI_EVENT_DATA_DUMP_REQUEST, NULL);
        check_jvmti_error(jvmti, err, "set event notification");
    } exitAgentMonitor(jvmti);
	*/
}

/* Callback for JVMTI_EVENT_VM_DEATH */
static void JNICALL
vmDeath(jvmtiEnv *jvmti, JNIEnv *env)
{
    jvmtiError          err;

    /* Make sure everything has been garbage collected */
    err = (*jvmti)->ForceGarbageCollection(jvmti);
    check_jvmti_error(jvmti, err, "force garbage collection");

    /* Disable events and dump the heap information */
    enterAgentMonitor(jvmti); {
		        
		//err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_DISABLE,
        //                    JVMTI_EVENT_DATA_DUMP_REQUEST, NULL);
        //check_jvmti_error(jvmti, err, "set event notification");

        dataDumpRequest(jvmti);
		
        gdata->vmDeathCalled = JNI_TRUE;
    } exitAgentMonitor(jvmti);
}

/* Add demo jar file to boot class path (the BCI Tracker class must be
 *     in the boot classpath)
 *
 */
void
add_demo_jar_to_bootclasspath2(jvmtiEnv *jvmti, char *demo_name)
{
    jvmtiError error;
    error = (*jvmti)->AddToBootstrapClassLoaderSearch(jvmti, (const char*)demo_name);
    check_jvmti_error(jvmti, error, "Cannot add to boot classpath");
}


/* Agent_OnLoad() is called first, we prepare for a VM_INIT event here. */
JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options, void *reserved)
{
    jint                rc;
    jvmtiError          err;
    jvmtiCapabilities   capabilities;
    jvmtiEventCallbacks callbacks;
    jvmtiEnv           *jvmti;

    /* Get JVMTI environment */
    jvmti = NULL;
    rc = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION);
    if (rc != JNI_OK) {
        fatal_error("ERROR: Unable to create jvmtiEnv, error=%d\n", rc);
        return -1;
    }
    if ( jvmti == NULL ) {
        fatal_error("ERROR: No jvmtiEnv* returned from GetEnv\n");
    }

	/* Here we save the jvmtiEnv* for Agent_OnUnload(). */
    gdata->jvmti = jvmti;

    /* Get/Add JVMTI capabilities */
    (void)memset(&capabilities, 0, sizeof(capabilities));
    capabilities.can_tag_objects = 1;
    capabilities.can_generate_garbage_collection_events = 1;
    err = (*jvmti)->AddCapabilities(jvmti, &capabilities);
    check_jvmti_error(jvmti, err, "add capabilities");

    /* Create the raw monitor */
    err = (*jvmti)->CreateRawMonitor(jvmti, "agent lock", &(gdata->lock));
    check_jvmti_error(jvmti, err, "create raw monitor");

    /* Set callbacks and enable event notifications */
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMStart          		  = &cbVMStart;
    callbacks.VMInit                  = &vmInit;
    callbacks.VMDeath                 = &vmDeath;
    callbacks.DataDumpRequest         = &dataDumpRequest;

    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    check_jvmti_error(jvmti, err, "set event callbacks");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                        JVMTI_EVENT_VM_INIT, NULL);
    check_jvmti_error(jvmti, err, "set event notifications");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                        JVMTI_EVENT_VM_DEATH, NULL);
    check_jvmti_error(jvmti, err, "set event notifications");
	err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                        JVMTI_EVENT_VM_START, NULL);
    check_jvmti_error(jvmti, err, "set event notifications");

	/* Add demo jar file to boot classpath */
    add_demo_jar_to_bootclasspath2(jvmti, "heapanalysis.jar");

	/* Setup global data */
    return 0;
}

/* Agent_OnUnload() is called last */
JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM *vm)
{
}
