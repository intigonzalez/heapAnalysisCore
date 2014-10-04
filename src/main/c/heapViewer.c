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
#include <errno.h>

#include <sys/time.h>

#include <dlfcn.h>

#include "jni.h"
#include "jvmti.h"

#include "agent_util.h"
#include "ResourceManagement.h"
#include "plugins.h"


#define MAX_NUMBER_OF_PLUGINS 100

/* Global static data */
typedef struct {
	jvmtiEnv      *jvmti;
    jboolean      vmDeathCalled;
    jboolean      dumpInProgress;
    jrawMonitorID lock;
    ClassInfo* info_classes;
	FILE* f;
	jint nbPlugins;
	HeapAnalyzerPlugin plugins[MAX_NUMBER_OF_PLUGINS];
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

static jobject
StandardCreateResults
(jvmtiEnv* jvmti, JNIEnv *jniEnv, char* principal_name, ClassDetails* details, int count_classes)
{
	jobjectArray resultForOnePrincipal = NULL;
	jobject singleResult;
	jobject finalResult;
	jclass classDetails;
	jclass classPrincipalDetails;
	jmethodID constructor;
	jmethodID constructor_classPrincipalDetails;
	int i;

	classDetails = (*jniEnv)->FindClass(jniEnv, "org/heapexplorer/heapanalysis/ClassDetailsUsage");
	constructor = (*jniEnv)->GetMethodID(jniEnv, classDetails, "<init>", "(Ljava/lang/String;II)V");

	classPrincipalDetails = (*jniEnv)->FindClass(jniEnv, "org/heapexplorer/heapanalysis/PrincipalClassDetailsUsage");
	constructor_classPrincipalDetails =
			(*jniEnv)->GetMethodID(jniEnv, classPrincipalDetails, "<init>",
				"(Ljava/lang/String;[Lorg/heapexplorer/heapanalysis/ClassDetailsUsage;)V");

	// create arrays
	resultForOnePrincipal = (*jniEnv)->NewObjectArray(jniEnv, count_classes, classDetails, NULL);
	for ( i = 0 ; i < count_classes; i++) {
		singleResult = (*jniEnv)->NewObject(jniEnv, classDetails, constructor, 
					(*jniEnv)->NewStringUTF(jniEnv, getClassSignature(&details[i])),details[i].count, details[i].space);
		(*jniEnv)->SetObjectArrayElement(jniEnv, resultForOnePrincipal, i, singleResult);	
	}

	finalResult = (*jniEnv)->NewObject(jniEnv, classPrincipalDetails, constructor_classPrincipalDetails,
                  					(*jniEnv)->NewStringUTF(jniEnv, principal_name),resultForOnePrincipal);

	return finalResult;
}

static jobject
KevoreeCreateResults
(jvmtiEnv* jvmti, JNIEnv *jniEnv, char* principal_name, ClassDetails* details, int count_classes)
{
	jobjectArray resultForOnePrincipal = NULL;
	jobject singleResult;
	jobject finalResult;
	jclass classDetails;
	jclass classPrincipalDetails;
	jmethodID constructor;
	jmethodID constructor_classPrincipalDetails;
	int i;
	jlong totalSpace = 0;

	classDetails = (*jniEnv)->FindClass(jniEnv, "org/heapexplorer/heapanalysis/ClassDetailsUsage");
	constructor = (*jniEnv)->GetMethodID(jniEnv, classDetails, "<init>", "(Ljava/lang/String;II)V");

	classPrincipalDetails = (*jniEnv)->FindClass(jniEnv, "org/heapexplorer/heapanalysis/PrincipalClassDetailsUsage");
	constructor_classPrincipalDetails =
			(*jniEnv)->GetMethodID(jniEnv, classPrincipalDetails, "<init>",
				"(Ljava/lang/String;J[Lorg/heapexplorer/heapanalysis/ClassDetailsUsage;)V");

    if (constructor_classPrincipalDetails == NULL)
        fatal_error("ERROR: Impossible to obtain PrincipalClassDetailsUsage::<init> in Kevoree_principal\n");

	// create arrays
	resultForOnePrincipal = (*jniEnv)->NewObjectArray(jniEnv, 0, classDetails, NULL);
	for ( i = 0 ; i < count_classes; i++) {
	    totalSpace += details[i].space;
	}

	finalResult = (*jniEnv)->NewObject(jniEnv, classPrincipalDetails, constructor_classPrincipalDetails,
                  					(*jniEnv)->NewStringUTF(jniEnv, principal_name),totalSpace,resultForOnePrincipal);

	return finalResult;
}


/** 
 * Explore the memory consumption of each principal 
 * (follows different strategies) 
 */
static 
jobject explorePrincipals(
		jvmtiEnv* jvmti, 
		JNIEnv *jniEnv,
		int count_classes,
		jclass* classes,
		CreatePrincipals createPrincipals,
		CreateResults createResults)
{
    int j;
    int i;
	int count_principals;
	jlong tmp;
    ResourcePrincipal* principals;
    jvmtiError err;
	ClassDetails* details;
	ResourcePrincipal shared_stuff_principal;
	
	jobjectArray result = NULL;
	jclass classObject;
	jobject tmpObj = NULL;

	// remove this after debug
	details = (ClassDetails*)calloc(sizeof(ClassDetails), count_classes);
	for ( i = 0 ; i < count_classes ; i++ ) {
		details[i].info = &gdata->info_classes[i];
	}

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
	count_principals = createPrincipals(jvmti, jniEnv, &principals, gdata->info_classes , count_classes);
    // Step 2 (Empty if the resource principal is the whole JVM)
	
    // Step 3
	for ( i = 0 ; i < count_classes ; i++ ) {
			//jlong t_ptr;
			
           	/* Tag this jclass */
           	err = (*jvmti)->SetTag(jvmti, classes[i],
                          tagForObject(NULL));
           	check_jvmti_error(jvmti, err, "set object tag");

			// remove this after debuging
			
           	/* Tag this jclass */
           	//err = (*jvmti)->GetTag(jvmti, classes[i], &t_ptr);
           	//check_jvmti_error(jvmti, err, "get object tag");
			//setUserDataForTag(t_ptr, &(details[i]));
   	}

	// prepare array for results
	classObject = (*jniEnv)->FindClass(jniEnv, "java/lang/Object");
	result = (*jniEnv)->NewObjectArray(jniEnv, count_principals, classObject, NULL);

    for (j = 0 ; j < count_principals ; ++j) {
    	// step 3.1	
       	for ( i = 0 ; i < count_classes ; i++ ) {
				jlong t_ptr;
               	/* Tag this jclass */
               	err = (*jvmti)->GetTag(jvmti, classes[i], &t_ptr);
               	check_jvmti_error(jvmti, err, "get object tag");
				setUserDataForTag(t_ptr, &(principals[j].details[i]));
       	}

		// step 3.2 (Empty because all the roots are valid)
		// step 3.3
		/* Iterate through the heap and count up uses of jclass */
		((LocalExploration)(principals[j].strategy_to_explore))(jvmti,  &principals[j]);
		// step 3.4
		if (createResults == NULL)
			tmpObj= KevoreeCreateResults(jvmti, jniEnv, principals[j].name, principals[j].details, count_classes);
		else 
			tmpObj = createResults(jvmti, jniEnv, principals[j].user_data);

		(*jniEnv)->SetObjectArrayElement(jniEnv, result, j, tmpObj);
		
		//stdout_message("=> PRINTING INFO FOR: %s\n", principals[j].name);	

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
	return result;
}

static jobject JNICALL
do_analysis(jvmtiEnv *jvmti, JNIEnv *jniEnv, int id) {
	jobject result = NULL;
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
				//fprintf(stderr, "Pointer %p %s\n", gdata->info_classes[i].signature, sig);
				gdata->info_classes[i].is_clazz_clazz = !strcmp(sig, "Ljava/lang/Class;");
				deallocate(jvmti, sig);
			}


			if (id < gdata->nbPlugins) {
				
				stdout_message("Starting to explore\n");
				result = explorePrincipals(jvmti, // the jvmti environment  
							jniEnv, // jni environment
							count, // number of loaded classes 
							classes, // loaded classes
							gdata->plugins[id].createPrincipals, // strategy to create principals,
							gdata->plugins[id].createResults // strategy to create Results
						);
			}

		    /* Free up all allocated space */
		    deallocate(jvmti, classes);
		    for ( i = 0 ; i < count ; i++ ) {
			    if ( gdata->info_classes[i].signature != NULL )
		            free(gdata->info_classes[i].signature);
			}

			free(gdata->info_classes);
			gdata->info_classes = NULL;

            gdata->dumpInProgress = JNI_FALSE;
        }
    } exitAgentMonitor(jvmti);

	return result;
}

/* Callback for JVMTI_EVENT_DATA_DUMP_REQUEST (Ctrl-\ or at exit) */
static void JNICALL
dataDumpRequest(jvmtiEnv *jvmti)
{
    //do_analysis(jvmti, NULL, PER_THREAD_ANALYSIS | ALIVE_OBJECTS_ANALYSIS | WHOLE_JVM_ANALYSIS);
}

/* Java Native Method for analysis */
static jobject JNICALL
HEAPANALYSIS_native_analysis(JNIEnv *env, jclass klass, jint types)
{
	jint count;
	jthread* threads;
	jvmtiError err;
	jvmtiError* results;
	int i;

	return do_analysis(gdata->jvmti, env, types);
}

/* Java Native Method for count_of_analysis */
static jint JNICALL
HEAPANALYSIS_native_count_of_analysis(JNIEnv *env, jclass klass)
{
	jint r;
	enterAgentMonitor(gdata->jvmti); {
		r = gdata->nbPlugins;
	} exitAgentMonitor(gdata->jvmti);
	return r;
}

/* Java Native Method for getAnalysis */
static jobject JNICALL
HEAPANALYSIS_native_getAnalysis(JNIEnv *env, jclass klass, jint index)
{
	//enterAgentMonitor(gdata->jvmti); {
	if (index < 0 || index >= gdata->nbPlugins) {
		// throw exception	
		return NULL;	
	}
	else {
		jmethodID cnstrctr;
		jstring name;
		jstring desc;
    	jclass c = (*env)->FindClass(env, "org/heapexplorer/heapanalysis/AnalysisType");
    	if (c == 0) {
        	// throw exception	
			return NULL;
     	}
		cnstrctr = (*env)->GetMethodID(env, c, "<init>", "(ILjava/lang/String;Ljava/lang/String;)V");
    	if (cnstrctr == 0) {
        	// throw exception	
			return NULL;
    	}
		name = (*env)->NewStringUTF(env, gdata->plugins[index].name);
		desc = (*env)->NewStringUTF(env, gdata->plugins[index].description);
		return (*env)->NewObject(env, c, cnstrctr, index, name, desc);
	}
	//} exitAgentMonitor(gdata->jvmti);
}

static void JNICALL
prepareClass(JNIEnv *env, jclass klass)
{
	int rc;
	/* Java Native Methods for class */
    static JNINativeMethod registry[3] = {
        {"analysis", "(I)Ljava/lang/Object;",
            (void*)&HEAPANALYSIS_native_analysis},
		{"count_of_analysis", "()I",
            (void*)&HEAPANALYSIS_native_count_of_analysis},
		{"getAnalysis", "(I)Lorg/heapexplorer/heapanalysis/AnalysisType;",
            (void*)&HEAPANALYSIS_native_getAnalysis},
	
    };

	rc = (*env)->RegisterNatives(env, klass, registry, 3);
    if ( rc != 0 ) {
        fatal_error("ERROR: JNI: Cannot register native methods for %s\n",
                    "org/heapexplorer/heapanalysis/HeapAnalysis");
    }
}

/* Callback for JVMTI_EVENT_VM_START */
static void JNICALL
cbVMStart(jvmtiEnv *jvmti, JNIEnv *env)
{
    enterAgentMonitor(jvmti); {
        jclass   klass;

        /* The VM has started. */
        stdout_message("VMStart\n");

        /* Register Natives for class whose methods we use */
        klass = (*env)->FindClass(env, "org/heapexplorer/heapanalysis/HeapAnalysis");
        if ( klass == NULL ) {
            fatal_error("ERROR: JNI: Cannot find %s with FindClass\n",
                        "org/heapexplorer/heapanalysis/HeapAnalysis");
        }
        
		prepareClass(env, klass);
		stdout_message("VMStart DONE\n");

    } exitAgentMonitor(jvmti);
}

void JNICALL
onClassLoad(jvmtiEnv *jvmti,
            JNIEnv* env,
            jthread thread,
            jclass klass)
{
	enterAgentMonitor(jvmti); {
		char* cName;
		char* gName;
		jvmtiError error;
		

		error = (*jvmti)->GetClassSignature(jvmti,
            		klass,
            		&cName,
            		&gName);

		check_jvmti_error(jvmti, error, "Cannot get class name");
		if (strcmp(cName, "Lorg/heapexplorer/heapanalysis/HeapAnalysis;")==0) {
			stdout_message("OHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH, no, loading the class again\n");
		    prepareClass(env, klass);
		}
		(*jvmti)->Deallocate(jvmti, gName);
		(*jvmti)->Deallocate(jvmti, cName);
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

/*
	Load the plugins declared in the configuration file
*/
static void
loadPlugins(const char* configFile)
{
	FILE* f;
	char buffer[255];
	if ((f = fopen(configFile, "r")) == NULL) {
		// no plugins to load	
		fprintf(stderr, "Error opening the file errno=%d (%s). File %s\n", errno, strerror(errno), configFile);
		return;
	}
	while (fgets (buffer, sizeof(buffer), f)) {
		void *handle;
		PluginDeclareFunction declarePlugin;
		int n = strlen(buffer);
		if (buffer[n-1] == '\n') buffer[n-1] = 0;
    	stdout_message("Loading plugin: %s\n", buffer);		
		handle = dlopen(buffer, RTLD_NOW | RTLD_LOCAL);
		if (!handle) {
        	fprintf(stderr, "Error loading the plugin=%s, error=%s\n", buffer, dlerror());
        	continue;
    	}
		dlerror();    /* Clear any existing error */
		
		*(void **) (&declarePlugin) = dlsym(handle, "declarePlugin"); // do you really get this cast? hahahahaha
		gdata->plugins[gdata->nbPlugins].createResults = NULL;
		(*declarePlugin)(&gdata->plugins[gdata->nbPlugins]);
		//stdout_message("Plugin Name: %s\nPlugin Description: %s\n", gdata->plugins[gdata->nbPlugins].name, gdata->plugins[gdata->nbPlugins].description);
		gdata->nbPlugins++;
  	}
	fclose(f);
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

	gdata->f = stdout;
	if (options != NULL && strlen(options) > 0) {
		gdata->f = fopen(options, "w");
		stdout_message("This is shit %d\n", 1);
	}

	gdata->nbPlugins = 0;
	loadPlugins("/tmp/config.ini");
	

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
	capabilities.can_suspend = 1;
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
	callbacks.ClassLoad				  = &onClassLoad;

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
	err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                          JVMTI_EVENT_CLASS_LOAD, NULL);
    check_jvmti_error(jvmti, err, "Cannot set event notification");

	/* Add demo jar file to boot classpath */
    add_demo_jar_to_bootclasspath2(jvmti, "/tmp/heapanalysis.jar");

	/* Setup global data */
    return 0;
}

/* Agent_OnUnload() is called last */
JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM *vm)
{
	fclose(gdata->f);
}
