
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "plugins.h"

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

static void
include(SetOfStrings* set, char* s)
{
	set->elements[set->cardinal++] = s;
}



/* Get a group name for a jthread */
static void
get_thread_group_name(jvmtiEnv *jvmti, jthread thread, char *tname, int maxlen)
{
    jvmtiThreadInfo info;
    jvmtiError      error;

    /* Make sure the stack variables are garbage free */
    (void)memset(&info,0, sizeof(info));

    /* Assume the name is unknown for now */
    (void)strcpy(tname, "Unknown");

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
		    len = (int)strlen(infoG.name);
		    if ( len < maxlen ) {
		        (void)strcpy(tname, infoG.name);
		    }
			if (infoG.name != NULL)			
				deallocate(jvmti, (void*)infoG.name);
		}

        /* Every string allocated by JVMTI needs to be freed */
		if (info.name != NULL)        
			deallocate(jvmti, (void*)info.name);
		
    }
}

static jboolean startsWith(char* prefix, char* s)
{
	while((*s) != 0 && (*prefix)!=0) {
		if ((*s) != (*prefix))
			return JNI_FALSE;	
		s++;
		prefix++;
	}
	return (*prefix) == 0;
}
static jboolean 
isSystemClass(char* className)
{
	if (className[0] == 'L')
		return startsWith("Ljava/", className)
				|| startsWith("Ljavax/", className)
				|| startsWith("Lsun/", className);
	else if (className[0] == '[') {
	return startsWith("[C", className)
		|| startsWith("[B", className)
		|| startsWith("[Z", className)
		|| startsWith("[J", className)
		|| startsWith("[Lsun/", className)
		|| startsWith("[Ljava/", className)
		|| startsWith("[Ljavax/", className)		
		;
	}
	return JNI_FALSE;
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
							&& isSystemClass(getClassSignature(d))) {
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
				return 0; // ignore it		
			}
			else if (!isTagged(*tag_ptr)) {
					// It is neither a class object nor an object I already visited, so follow references and account of it
					d->count++;
					d->space += (int)size;
					*tag_ptr = tagForObject(princ);	
				
					return JVMTI_VISIT_OBJECTS;
			}
	}
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
	
	if (class_tag == (jlong)0) return 0;

	d = (ClassDetails*)getDataFromTag(class_tag);
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
				return JVMTI_VISIT_OBJECTS;			
			}
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
						&& !isSystemClass(getClassSignature(d))) {								
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
				return 0; // ignore it		
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

/* create principals */
jint createPrincipal(jvmtiEnv* jvmti, JNIEnv *jniEnv,
		ResourcePrincipal** principals, ClassInfo* infos, int count_classes)
{
	jint count_principals, thread_count;
	int j;
	int i;
	int k;
	jthread* threads;
	jvmtiError err;
	SetOfStrings strings;

	memset(&strings,0, sizeof(SetOfStrings));

	err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
	check_jvmti_error(jvmti, err, "get all threads");
	count_principals = 1; 
	for (i = 0 ; i < thread_count ; ++i) {
		char  tname[255];

        get_thread_group_name(jvmti, threads[i], tname, sizeof(tname));
        if (startsWith("another guy", tname) && indexOf(&strings, tname) == -1) {
			include(&strings, strdup(tname));
			count_principals++; // increase count
		}
	}
	
	(*principals) = (ResourcePrincipal*)calloc(sizeof(ResourcePrincipal), count_principals);

	(*principals)[0].details = (ClassDetails*)calloc(sizeof(ClassDetails), count_classes);
    if ( (*principals)[0].details == NULL ) 
           	fatal_error("ERROR: Ran out of malloc space\n");

    for ( i = 0 ; i < count_classes ; i++ )
		(*principals)[0].details[i].info = &infos[i];

	(*principals)[0].name = "system-info";
	(*principals)[0].tag = 1;
	(*principals)[0].strategy_to_explore = &followReferences_to_discard;

	for (i = 0 ; i < thread_count ; ++i) {
		jvmtiThreadInfo t_info;
		jint index;
		char  tname[255];

		get_thread_group_name(jvmti, threads[i], tname, sizeof(tname));
		if (!startsWith("another guy", tname)) continue;

		index = indexOf(&strings, tname);
		j = index + 1;

		//printf("////////////////////////////////////////////////////////////\n");
		
		if ((*principals)[j].details == NULL) {
			(*principals)[j].name = strdup(tname);
			/* Setup an area to hold details about these classes */
			(*principals)[j].details = (ClassDetails*)calloc(sizeof(ClassDetails), count_classes);
			if ( (*principals)[j].details == NULL ) 
			       	fatal_error("ERROR: Ran out of malloc space\n");

			for ( k = 0 ; k < count_classes ; k++ )
				(*principals)[j].details[k].info = &infos[k];

		
			(*principals)[j].strategy_to_explore = &explore_FollowReferences_Thread;
			(*principals)[j].tag = (jlong)(j+1);
		}

		/* Tag this jthread */
    	err = (*jvmti)->SetTag(jvmti, 
							threads[i],
                        	tagForObject( &(*principals)[j]) );
    	check_jvmti_error(jvmti, err, "set thread tag");

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


	}
	/* Free up all allocated space */
    deallocate(jvmti, threads);

	return count_principals;
}

/** Fill a structure with the infomation about the plugin 
* Returns 0 if everything was OK, a negative value otherwise	
*/
int DECLARE_FUNCTION(HeapAnalyzerPlugin* r)
{
	r->name = "threadgroup_principal";
	r->description = "This plugin caclulates the number of objects of each class that are"
 		"reachable from all threads that belong to some thread group with a name begining by 'another guy'."
		"Each of such thread group represents a diferent resource principal";
	r->createPrincipals = createPrincipal;
	return 0;
}
