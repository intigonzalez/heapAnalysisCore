
#include "thread_principal.h"


/* Get a name for a jthread */
static void
get_thread_name(jvmtiEnv *jvmti, jthread thread, char *tname, int maxlen)
{
    jvmtiThreadInfo info;
    jvmtiError      error;

    /* Make sure the stack variables are garbage free */
    (void)memset(&info,0, sizeof(info));

    /* Assume the name is unknown for now */
    (void)strcpy(tname, "Unknown");

    /* Get the thread information, which includes the name */
    error = (*jvmti)->GetThreadInfo(jvmti, thread, &info);
    check_jvmti_error(jvmti, error, "Cannot get thread info");

    /* The thread might not have a name, be careful here. */
    if ( info.name != NULL ) {
        int len;

        /* Copy the thread name into tname if it will fit */
        len = (int)strlen(info.name);
        if ( len < maxlen ) {
            (void)strcpy(tname, info.name);
        }

        /* Every string allocated by JVMTI needs to be freed */
        deallocate(jvmti, (void*)info.name);
    }
}

#define STRLEN(s) (strlen(s))
#define STRNCMP(s1,s2) strncmp(s1,s2,STRLEN(s1))
static jboolean 
isSystemClass(char* className)
{
	return STRNCMP("Ljava/", className)==0
		|| STRNCMP("Lsun/", className)==0
		|| STRNCMP("[C", className)==0
		|| STRNCMP("[B", className)==0
		|| STRNCMP("[Z", className)==0
		|| STRNCMP("[J", className)==0
		|| STRNCMP("[Lsun/", className)==0
		|| STRNCMP("[Ljava/", className)==0
		|| STRNCMP("[Ljavax/", className)==0
		|| STRNCMP("Ljava/", className)==0		
		;
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

	if ( (class_tag != (jlong)0)) {
		ClassDetails *d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
		char* className = getClassSignature(d);
		
		switch(reference_kind) {
			//case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
			//case JVMTI_HEAP_REFERENCE_OTHER:
			//case JVMTI_HEAP_REFERENCE_MONITOR:
			//case JVMTI_HEAP_REFERENCE_SYSTEM_CLASS:
			//case JVMTI_HEAP_REFERENCE_JNI_GLOBAL:
			//					return 0;
			case JVMTI_HEAP_REFERENCE_THREAD:
				if (isTagged(*tag_ptr)) // if tagged from another principal
					return 0;		
				d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
				d->count++;
				d->space += (int)size;
				*(tag_ptr) = princ->tag;
				return JVMTI_VISIT_OBJECTS;
			case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
				if (isTagged(reference_info->stack_local.thread_tag) && 
						!isTaggedByPrincipal(reference_info->stack_local.thread_tag, princ)) // if tagged for a different principal 
					return 0;
			default:
				if (isClassClass(d)) {
					// it's a class object. I have to discover if I already visited it
					if (!isTagged(*tag_ptr)) {
						return 0;			
					}
					
					d = (ClassDetails*)(void*)(ptrdiff_t)(*tag_ptr);
					//if (!strcmp(getClassSignature(d), "LAnother;")) {
					//	stdout_message("One Another class detected in the common stage with ref_kind:%d\n", reference_kind);					
					//}					
					if (!isClassVisited(d) 
								&& isSystemClass(getClassSignature(d))) {
						setClassInfoVisited(d, JNI_TRUE);						
						d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
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
				else {
					if (!isTagged(*tag_ptr)) {
						// It it neither a class object nor an object I already visited, so follow references and account of it
						d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
						d->count++;
						d->space += (int)size;
						*tag_ptr = princ->tag;	
					
						return JVMTI_VISIT_OBJECTS;
					}
				}
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

	if ( (class_tag != (jlong)0)) {
		ClassDetails *d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
		switch(reference_kind) {
			case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
			case JVMTI_HEAP_REFERENCE_OTHER:
			case JVMTI_HEAP_REFERENCE_MONITOR:
			case JVMTI_HEAP_REFERENCE_SYSTEM_CLASS:
			case JVMTI_HEAP_REFERENCE_JNI_GLOBAL:
			//case 8: // don't follow static fields
								return 0;
			case JVMTI_HEAP_REFERENCE_THREAD:
				if (isTaggedByPrincipal(*tag_ptr, princ)) {
					d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
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
					if (!isTagged(*tag_ptr)) {
						return 0;			
					}
					
					d = (ClassDetails*)(void*)(ptrdiff_t)(*tag_ptr);
					if (!isClassVisited(d)
							&& !isSystemClass(getClassSignature(d))) {						
						setClassInfoVisited(d, JNI_TRUE);						
						//stdout_message("Clase de mierda %s, ref_kind %d\n", getClassSignature(d), reference_kind);							
						d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
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
				else {
					if (!isTagged(*tag_ptr)) {
						// It it neither a class object nor an object I already visited, so follow references and account of it
						d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
						d->count++;
						d->space += (int)size;
						*tag_ptr = princ->tag;
					
						return JVMTI_VISIT_OBJECTS;
					}
				}
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
jint createPrincipal_per_thread(jvmtiEnv* jvmti, 
		ResourcePrincipal** principals, ClassInfo* infos, int count_classes)
{
	jint count_principals, thread_count;
	int j;
	int i;
	jthread* threads;
	jvmtiError err;
	jlong tmp;

	err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
	check_jvmti_error(jvmti, err, "get all threads");
	count_principals = 1; 
	tmp = 0;
	for (i = 0 ; i < thread_count ; ++i) {
		char  tname[255];

        get_thread_name(jvmti, threads[i], tname, sizeof(tname));
        if (!strcmp(tname, "The guy")) {
			jvmtiThreadInfo th_info;
			count_principals++; // increase count
			/* Tag this jthread */
        	err = (*jvmti)->SetTag(jvmti, threads[i],
                            	(jlong)(tmp + count_principals));
        	check_jvmti_error(jvmti, err, "set thread tag");

			err = (*jvmti)->GetThreadInfo(jvmti, 
							threads[i], &th_info);
			check_jvmti_error(jvmti, err, "get thread info");
			
			//err = (*jvmti)->SetTag(jvmti, th_info.context_class_loader,
            //                	(jlong)(tmp + count_principals));
        	//check_jvmti_error(jvmti, err, "set thread tag");
		}
	}
	/* Free up all allocated space */
    deallocate(jvmti, threads);

	(*principals) = (ResourcePrincipal*)calloc(sizeof(ResourcePrincipal), count_principals);         
    for (j = 0 ; j < count_principals ; ++j) {
		/* Setup an area to hold details about these classes */
		(*principals)[j].details = (ClassDetails*)calloc(sizeof(ClassDetails), count_classes);
        if ( (*principals)[j].details == NULL ) 
               	fatal_error("ERROR: Ran out of malloc space\n");

        for ( i = 0 ; i < count_classes ; i++ )
			(*principals)[j].details[i].info = &infos[i];

		(*principals)[j].tag = (++tmp);
		(*principals)[j].strategy_to_explore = (j==0)?
						(&followReferences_to_discard):(&explore_FollowReferences_Thread);

//		(*principals)[j].strategy_to_explore = (&explore_FollowReferences_Thread);
    }
	return count_principals;
}
