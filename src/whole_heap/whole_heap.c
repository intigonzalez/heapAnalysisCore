#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../plugins.h"

/* Heap object callback */
static jint JNICALL
cbHeapObject(jlong class_tag, jlong size, jlong* tag_ptr, jint length,
           void* user_data)
{
    if ( class_tag != (jlong)0 ) {
        ClassDetails *d;

        d = (ClassDetails*)getDataFromTag(class_tag);
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

/* create principals */
jint  createPrincipal(jvmtiEnv* jvmti, 
		ResourcePrincipal** principals, ClassInfo* infos, int count_classes)
{
	jint count_principals;
	int j;
	int i;
	jlong tmp;

	count_principals = 1;
	(*principals) = (ResourcePrincipal*)calloc(sizeof(ResourcePrincipal), count_principals);        
    for (j = 0 ; j < count_principals ; ++j) {
		/* Setup an area to hold details about these classes */
		(*principals)[j].details = (ClassDetails*)calloc(sizeof(ClassDetails), count_classes);
        if ( (*principals)[j].details == NULL ) 
               	fatal_error("ERROR: Ran out of malloc space\n");

        for ( i = 0 ; i < count_classes ; i++ )
			(*principals)[j].details[i].info = &infos[i];

		(*principals)[j].tag = (j+1);
		(*principals)[j].strategy_to_explore = &explore_all_objects;
    }
	return count_principals;
}

/** Fill a structure with the infomation about the plugin 
* Returns 0 if everything was OK, a negative value otherwise	
*/
int DECLARE_FUNCTION(HeapAnalyzerPlugin* r)
{
	r->name = "whole_heap_exploration";
	r->description = "This plugin calculates the number of objects of each class within the whole JVM. It returns boths, alive and dead objects";
	r->createPrincipals = &createPrincipal;
	return 0;
}
