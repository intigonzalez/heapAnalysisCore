
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "plugins.h"

#define PREFIX_KEV_GROUP "kev/"

typedef struct {
	int cardinal;
	char* elements[100];
	
} SetOfStrings;

static jint
indexOf( SetOfStrings* set, char* s)
{
	int i;
	for (i = 0 ; i < set->cardinal ; i++) {
		if (!strcmp(s, set->elements[i]))
			return i;	
	}
	return -1;
}

static int
include(SetOfStrings* set, char* s)
{
	set->elements[set->cardinal] = s;
	return set->cardinal++;
}



/* Get a group name for a jthread */
static void
get_thread_group_name(jvmtiEnv *jvmti, jthread thread, char *tname, int maxlen)
{
    jvmtiThreadInfo info;
    jvmtiError      error;

    /* Make sure the stack variables are garbage free */
    (void)memset(&info,0, sizeof(info));

    /* Get the thread information, which includes the thread group */
    error = (*jvmti)->GetThreadInfo(jvmti, thread, &info);
    check_jvmti_error(jvmti, error, "Cannot get thread info");

    /* The thread might not have a name, be careful here. */
    if ( info.thread_group != NULL ) {
        int len;
		jvmtiThreadGroupInfo infoG;
		(void)memset(&infoG,0, sizeof(infoG));

		error = (*jvmti)->GetThreadGroupInfo(jvmti, info.thread_group, &infoG);
    	check_jvmti_error(jvmti, error, "Cannot get thread group info");
		if (infoG.name != NULL) {
		    /* Copy the thread name into tname if it will fit */
		    (void)strcpy(tname, infoG.name);
			if (infoG.name != NULL)			
				deallocate(jvmti, (void*)infoG.name);
		}
		else tname[0] = '0';

        /* Every string allocated by JVMTI needs to be freed */
		if (info.name != NULL)        
			deallocate(jvmti, (void*)info.name);
    }
    else tname[0] = '0';
}

/* Callback for HeapReferences in FollowReferences (Memory consumed by Thread) */
static 
jint JNICALL callback_all_references
    (jvmtiHeapReferenceKind reference_kind, 
     const jvmtiHeapReferenceInfo* reference_info, 
     jlong class_tag, 
     jlong referrer_class_tag, 
     jlong size, 
     jlong* tag_ptr, 
     jlong* referrer_tag_ptr, 
     jint length, 
     void* user_data)
{	
	ClassDetails *d;
	ResourcePrincipal* princ = (ResourcePrincipal*)user_data;

	if (class_tag == (jlong)0) return 0;

	d = (ClassDetails*)getDataFromTag(class_tag);
	switch(reference_kind) {
		case JVMTI_HEAP_REFERENCE_THREAD:
			if (isTagged(*tag_ptr)) // if tagged from another principal
				return 0;

			d->count++;
			d->space += (int)size;
			*(tag_ptr) = tagForObject(princ);
//			printf("PINGA EL TIPO ESTA AQUI (33333) %d, %s\n", d->space, princ->name);
			return JVMTI_VISIT_OBJECTS;
		case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
			if (isTagged(reference_info->stack_local.thread_tag) && 
					!isTaggedByPrincipal(reference_info->stack_local.thread_tag, princ)) // if tagged for a different principal 
				return 0;
		default:
			if (isClassClass(d)) {
				// it's a class object. I have to discover if I already visited it
				if ((*tag_ptr) == 0) return 0;			
				
				d = (ClassDetails*)getDataFromTag(*tag_ptr);		
				if (!isTagged(*tag_ptr) 
							&& isSystemClass(d)) {
					attachToPrincipal(*tag_ptr, princ);						
					d = (ClassDetails*)getDataFromTag(class_tag);
					d->count++;
					d->space += (int)size;						
					return JVMTI_VISIT_OBJECTS;
				}
				return 0;
			}
			else if (isTagged(*tag_ptr)) {
				// it is an object already visited by this resource principal, or
				// it is an object already visited for another resource principal in this iteration
				if (!isTaggedByPrincipal(*tag_ptr, princ))
					return 0; // ignore it
				else if (!mustFollowReferences(*tag_ptr)) {
					return 0; // ignore it
				}
				else {
					setFollowReferences(*tag_ptr, false);
					d = (ClassDetails*)getDataFromTag(class_tag);
					d->count++;
					d->space += (int)size;
					return JVMTI_VISIT_OBJECTS;
				}		
			}
			else if (!isTagged(*tag_ptr)) {
					// It is neither a class object nor an object I already visited, so follow references and account of it
					d->count++;
					d->space += (int)size;
					*tag_ptr = tagForObject(princ);	
				
					return JVMTI_VISIT_OBJECTS;
			}
	}
	//if (lla)
	//	printf("\n OHH< NOT NICE \n");
	return JVMTI_VISIT_OBJECTS;
}

/* Callback for HeapReferences in FollowReferences (Memory consumed by Thread) */
static
jint JNICALL callback_single_thread
    (jvmtiHeapReferenceKind reference_kind, 
     const jvmtiHeapReferenceInfo* reference_info, 
     jlong class_tag, 
     jlong referrer_class_tag, 
     jlong size, 
     jlong* tag_ptr, 
     jlong* referrer_tag_ptr, 
     jint length, 
     void* user_data)
{
	ClassDetails *d;
	ResourcePrincipal* princ = (ResourcePrincipal*)user_data;
	jboolean lla = false;
	
	if (class_tag == (jlong)0) return 0;

	d = (ClassDetails*)getDataFromTag(class_tag);
	//if (strcmp("Lorg/kevoree/microsandbox/samples/memory/MemoryConsumerMaxSize;", getClassSignature(d)) == 0) {
	//	printf("\nAMAZING in comp=%s tagged=%d taggedByMe=%d ref_kind=%d\n", princ->name, isTagged(*tag_ptr), isTaggedByPrincipal(*tag_ptr, princ), reference_kind);
	//	lla = true;
	//}
	switch(reference_kind) {
		case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
		case JVMTI_HEAP_REFERENCE_OTHER:
		case JVMTI_HEAP_REFERENCE_MONITOR:
		case JVMTI_HEAP_REFERENCE_SYSTEM_CLASS:
		case JVMTI_HEAP_REFERENCE_JNI_GLOBAL:
				return 0;
		case JVMTI_HEAP_REFERENCE_THREAD:
			if (isTaggedByPrincipal(*tag_ptr, princ)) {
				d->count++;
				d->space += (int)size;
//				printf("PINGA EL TIPO ESTA AQUI (con buen tag) %d, %s\n", d->space, princ->name);
				return JVMTI_VISIT_OBJECTS;
			}
//			printf("PINGA EL TIPO ESTA AQUI (22222) %d, %s\n", d->space, princ->name);
			return 0;
		case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
			if (!isTagged(reference_info->stack_local.thread_tag) ||
				!isTaggedByPrincipal(reference_info->stack_local.thread_tag, princ))
				return 0;
		default:
			if (isClassClass(d)) {
				// it's a class object. I have to discover if I already visited it
				if ((*tag_ptr) == 0) return 0;
				
				d = (ClassDetails*)getDataFromTag(*tag_ptr);
				if (!isTagged(*tag_ptr)
						&& !isSystemClass(d)) {
					attachToPrincipal(*tag_ptr, princ);
					
					d = (ClassDetails*)getDataFromTag(class_tag);
					d->count++;
					d->space += (int)size;							

					return JVMTI_VISIT_OBJECTS;
				}
				return 0;
			}
			else if (isTagged(*tag_ptr)) {
				// it is an object already visited by this resource principal, or
				// it is an object already visited by another resource principal in this iteration
				if (!isTaggedByPrincipal(*tag_ptr, princ))
					return 0; // ignore it
				else if (!mustFollowReferences(*tag_ptr)) {
					return 0; // ignore it
				}
				else {
					setFollowReferences(*tag_ptr, false);
					d = (ClassDetails*)getDataFromTag(class_tag);
					d->count++;
					d->space += (int)size;
					return JVMTI_VISIT_OBJECTS;
				}
			}
			else if (!isTagged(*tag_ptr)) {
				// It it neither a class object nor an object I already visited, so follow references and account of it
				*tag_ptr = tagForObject(princ);
				d->count++;
				d->space += (int)size;
						
				return JVMTI_VISIT_OBJECTS;
			}
	}
	return 0;
}

static
jint JNICALL callback_fields_of_thread
    (jvmtiHeapReferenceKind reference_kind, 
     const jvmtiHeapReferenceInfo* reference_info, 
     jlong class_tag, 
     jlong referrer_class_tag, 
     jlong size, 
     jlong* tag_ptr, 
     jlong* referrer_tag_ptr, 
     jint length, 
     void* user_data)
{	
	
	if (JVMTI_HEAP_REFERENCE_FIELD == reference_kind) {
		/*
		ClassDetails* d = (ClassDetails*)getDataFromTag(class_tag);
		ClassDetails* dR = (ClassDetails*)getDataFromTag(referrer_class_tag);
			
		printf("ref_kind=%d class_name=%s size=%ld referrer_class=%s \n", 
				reference_kind, 
				getClassSignature(d),
				size,
				getClassSignature(dR));
		*/
		
		ResourcePrincipal* princ = (ResourcePrincipal*)user_data;
		*tag_ptr = tagForObject(princ);

		setFollowReferences(*tag_ptr, true);
		
		//printf("\n\n kkk \n\n");
		//return JVMTI_VISIT_OBJECTS;
	}
	return 0; // stop traversing
}

/* Routine to explore a single thread */
static void
explore_FollowReferences_Thread(
		jvmtiEnv* jvmti, ResourcePrincipal* principal)
{
	jvmtiError err;
	jvmtiHeapCallbacks heapCallbacks;

	(void)memset(&heapCallbacks, 0, sizeof(heapCallbacks));
    heapCallbacks.heap_reference_callback = &callback_single_thread;
    err = (*jvmti)->FollowReferences(jvmti,
                   JVMTI_HEAP_FILTER_CLASS_UNTAGGED, NULL, NULL,
                   &heapCallbacks, (const void*)principal);
    check_jvmti_error(jvmti, err, "iterate through heap");
}

/* Routine to explore a single thread */

static void
followReferences_to_discard(
		jvmtiEnv* jvmti, ResourcePrincipal* principal)
{
	jvmtiError err;
	jvmtiHeapCallbacks heapCallbacks;

	(void)memset(&heapCallbacks, 0, sizeof(heapCallbacks));
    heapCallbacks.heap_reference_callback = &callback_all_references;
    err = (*jvmti)->FollowReferences(jvmti,
                   JVMTI_HEAP_FILTER_CLASS_UNTAGGED, NULL, NULL,
                   &heapCallbacks, (const void*)principal);
    check_jvmti_error(jvmti, err, "iterate through heap");
}

#define HEAP_ANALYSIS_CLASS "org/heapexplorer/heapanalysis/HeapAnalysis"

#define UpcallGetObjects_Class "org/heapexplorer/heapanalysis/UpcallGetObjects"
#define UpcallGetObjects_Sig "Lorg/heapexplorer/heapanalysis/UpcallGetObjects;"



static jclass klass_HeapAnalysis;
static jclass klass_UpcallGetObjects;
static jfieldID field_upcall;
static jmethodID method_getJavaDefinedObjects;
static jmethodID method_mustAnalyse;
static jobject object_upcall;
static jboolean cached;

static void
cacheMethodIds(JNIEnv *jniEnv)
{
    jclass cls1;
    jobject tmp;

    if (!cached) {
        cls1 = (*jniEnv)->FindClass(jniEnv, HEAP_ANALYSIS_CLASS);
        if (cls1 == NULL) fatal_error("ERROR: Impossible to obtain HeapAnalysis in Kevoree_principal\n");
        klass_HeapAnalysis = (*jniEnv)->NewGlobalRef(jniEnv, cls1);

        field_upcall = (*jniEnv)->GetStaticFieldID(jniEnv, klass_HeapAnalysis,
            "callback", UpcallGetObjects_Sig);

        if (field_upcall == NULL)
            fatal_error("ERROR: Impossible to obtain HeapAnalysis::callback in Kevoree_principal\n");

        tmp = (*jniEnv)->GetStaticObjectField(jniEnv,
                        klass_HeapAnalysis, field_upcall);
        object_upcall = (*jniEnv)->NewGlobalRef(jniEnv, tmp);

        cls1 = (*jniEnv)->FindClass(jniEnv, UpcallGetObjects_Class);
        if (cls1 == NULL)
            fatal_error("ERROR: Impossible to obtain UpcallGetObjects in Kevoree_principal\n");
        klass_UpcallGetObjects = (*jniEnv)->NewGlobalRef(jniEnv, cls1);

        method_getJavaDefinedObjects = (*jniEnv)->GetMethodID(jniEnv,
                    klass_UpcallGetObjects, "getJavaDefinedObjects",
                    "(Ljava/lang/String;)[Ljava/lang/Object;");
        if (method_getJavaDefinedObjects == NULL)
            fatal_error("ERROR: Impossible to obtain UpcallGetObjects::getJavaDefinedObjects in Kevoree_principal\n");

        method_mustAnalyse = (*jniEnv)->GetMethodID(jniEnv,
                            klass_UpcallGetObjects, "mustAnalyse",
                            "(Ljava/lang/String;)Z");
                if (method_getJavaDefinedObjects == NULL)
                    fatal_error("ERROR: Impossible to obtain UpcallGetObjects::mustAnalyse in Kevoree_principal\n");

        cached = true;
    }
}

/* create principals */
static jint
createPrincipal(jvmtiEnv* jvmti,
		JNIEnv *jniEnv,
		ResourcePrincipal** principals, ClassInfo* infos, int count_classes)
{
	jint count_principals, thread_count;
	int j;
	int i;
	int k;
	jthread* threads;
	jvmtiError err;
	SetOfStrings strings;
	int* threadsToConsider;
	jvmtiHeapCallbacks heapCallbacks;
	jboolean flags;

    cacheMethodIds(jniEnv);

	// a very bad way of initializing the set
	memset(&strings,0, sizeof(SetOfStrings));

	err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
	check_jvmti_error(jvmti, err, "get all threads");
	
	count_principals = 1; 
	threadsToConsider = (int*)calloc(sizeof(int), thread_count);
	for (i = 0 ; i < thread_count ; ++i) {
		char  tname[255];

        get_thread_group_name(jvmti, threads[i], tname, sizeof(tname));
        flags = (*jniEnv)->CallBooleanMethod(jniEnv, object_upcall,
                        method_mustAnalyse, (*jniEnv)->NewStringUTF(jniEnv, tname));

		if (flags/*strstr(tname, PREFIX_KEV_GROUP)==tname && (strstr(tname, "components") != NULL)*/){
			threadsToConsider[i] = true;
			k = indexOf(&strings, tname);
		    if (k == -1) {
				k = include(&strings, strdup(tname));
				count_principals++; // increase count
			}
//            printf("Thread from thread group %s\n", tname);
			threadsToConsider[i] = k;
		}
		else threadsToConsider[i] = -1;
	}
	
	(*principals) = (ResourcePrincipal*)calloc(sizeof(ResourcePrincipal), count_principals);

	(*principals)[0].details = (ClassDetails*)calloc(sizeof(ClassDetails), count_classes);
    if ( (*principals)[0].details == NULL ) 
           	fatal_error("ERROR: Ran out of malloc space\n");

    for ( i = 0 ; i < count_classes ; i++ )
		(*principals)[0].details[i].info = &infos[i];

	(*principals)[0].name = "system-info";
	(*principals)[0].tag = getLastTagCycleBoundary() + 1;
	(*principals)[0].strategy_to_explore = &followReferences_to_discard;

	for (i = 0 ; i < thread_count ; ++i) {
		jvmtiThreadInfo t_info;
		jboolean firstThread;

//		get_thread_group_name(jvmti, threads[i], tname, sizeof(tname));
		k = threadsToConsider[i];
		if (k == -1) continue;

		j = k + 1;
		
		firstThread = (*principals)[j].details == NULL;

		if (firstThread) {
			(*principals)[j].name = strdup(strings.elements[k]);
			/* Setup an area to hold details about these classes */
			(*principals)[j].details = (ClassDetails*)calloc(sizeof(ClassDetails), count_classes);
			if ( (*principals)[j].details == NULL ) 
			       	fatal_error("ERROR: Ran out of malloc space\n");

			for ( k = 0 ; k < count_classes ; k++ )
				(*principals)[j].details[k].info = &infos[k];

		
			(*principals)[j].strategy_to_explore = &explore_FollowReferences_Thread;
			(*principals)[j].tag = (getLastTagCycleBoundary() + j+1);
		}

		/* Tag this jthread */
    	err = (*jvmti)->SetTag(jvmti, 
							threads[i],
                        	tagForObject( &(*principals)[j]) );
    	check_jvmti_error(jvmti, err, "set thread tag");

		/* tag fields of thread */
		(void)memset(&heapCallbacks, 0, sizeof(heapCallbacks));
		heapCallbacks.heap_reference_callback = &callback_fields_of_thread;
		err = (*jvmti)->FollowReferences(jvmti,
		               0, NULL, threads[i],
		               &heapCallbacks, (const void*)&(*principals)[j]);
		check_jvmti_error(jvmti, err, "iterate through heap");


		/* tag the ThreadGroup */
		if (firstThread) {
			memset(&t_info,0, sizeof(t_info));
			err = (*jvmti)->GetThreadInfo(jvmti, 
							threads[i], &t_info);
			check_jvmti_error(jvmti, err, "get thread info");
		
			err = (*jvmti)->SetTag(jvmti, 
								t_info.context_class_loader,
		                    	tagForObject( &(*principals)[j]));
			check_jvmti_error(jvmti, err, "set classloader tag");

			if (t_info.thread_group != NULL) {
				err = (*jvmti)->SetTag(jvmti, 
								t_info.thread_group,
		                    	tagForObject( &(*principals)[j]));
				check_jvmti_error(jvmti, err, "set classloader tag");
			}

			// java-defined root_objects
            if (object_upcall != NULL) {
                jthrowable throwable;
                jint array_length;
                jint idx;

                jarray r = (*jniEnv)->CallObjectMethod(jniEnv, object_upcall,
                            method_getJavaDefinedObjects, (*jniEnv)->NewStringUTF(jniEnv, (*principals)[j].name));

//                throwable = (*jniEnv)->ExceptionOccurred(jniEnv);
//                if (throwable != NULL) {
//                    (*jniEnv)->ExceptionDescribe(jniEnv);
//                    (*jniEnv)->ExceptionClear(jniEnv);
//                    fatal_error("ERROR: Calling UpcallGetObjects::getJavaDefinedObjects in Kevoree_principal\n");
//                }
                array_length = (*jniEnv)->GetArrayLength(jniEnv, r);
                for (idx = 0 ; idx < array_length; idx++) {
                    jobject element = (*jniEnv)->GetObjectArrayElement(jniEnv, r, idx);
                    jlong tag_tmp = tagForObject( &(*principals)[j] );

                    setFollowReferences(tag_tmp, true);

                    err = (*jvmti)->SetTag(jvmti,
                                    element,
                                    tag_tmp);
                    check_jvmti_error(jvmti, err, "set element tag");
                }

            }
		}
	}
	/* Free up all allocated space */
    deallocate(jvmti, threads);
	free(threadsToConsider);

	return count_principals;
}

/** Fill a structure with the infomation about the plugin 
* Returns 0 if everything was OK, a negative value otherwise	
*/
int DECLARE_FUNCTION(HeapAnalyzerPlugin* r)
{
	r->name = "kevoree_principal";
	r->description = "This plugin calculates the number of objects of each class that are"
 		"reachable from all threads that belong to some thread group with a name beginning by 'kev/'."
		"Each of such thread group represents a different resource principal";
	r->createPrincipals = createPrincipal;
    cached = false;
	return 0;
}
