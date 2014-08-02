#include "whole_heap_principal.h"

/* Heap object callback */
static jint JNICALL
cbHeapObject(jlong class_tag, jlong size, jlong* tag_ptr, jint length,
           void* user_data)
{
    if ( class_tag != (jlong)0 ) {
        ClassDetails *d;

        d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
        //(*((jint*)(user_data)))++;
        d->count++;
        d->space += (int)size;
    }
    return JVMTI_VISIT_OBJECTS;
}

/* Routine to explore all alive objects within the JVM */
static
void explore_all_objects(
		jvmtiEnv* jvmti, ResourcePrincipal* principal)
{
	jvmtiError err;
	jvmtiHeapCallbacks heapCallbacks;
	(void)memset(&heapCallbacks, 0, sizeof(heapCallbacks));
    heapCallbacks.heap_iteration_callback = &cbHeapObject;
    err = (*jvmti)->IterateThroughHeap(jvmti,
                    JVMTI_HEAP_FILTER_CLASS_UNTAGGED, NULL,
	       	&heapCallbacks, NULL);
    check_jvmti_error(jvmti, err, "iterate through heap");
}

jint  create_single_principal(jvmtiEnv* jvmti, 
		ResourcePrincipal** principals, ClassInfo* infos, int count_classes)
{
	jint count_principals;
	int j;
	int i;
	jlong tmp;

	count_principals = 1;
	(*principals) = (ResourcePrincipal*)calloc(sizeof(ResourcePrincipal), count_principals); 
	tmp = getLastInSequence(); // tag for each principal        
    for (j = 0 ; j < count_principals ; ++j) {
		/* Setup an area to hold details about these classes */
		(*principals)[j].details = (ClassDetails*)calloc(sizeof(ClassDetails), count_classes);
        if ( (*principals)[j].details == NULL ) 
               	fatal_error("ERROR: Ran out of malloc space\n");

        for ( i = 0 ; i < count_classes ; i++ )
			(*principals)[j].details[i].info = &infos[i];

		(*principals)[j].previous_iteration_tag = tmp;
		(*principals)[j].tag = nextInSequence();
		(*principals)[j].strategy_to_explore = &explore_all_objects;
    }
	return count_principals;
}
