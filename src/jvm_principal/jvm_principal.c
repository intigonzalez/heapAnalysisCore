
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../plugins.h"


/* Callback for HeapReferences in FollowReferences (Shows alive objects in the whole JVM) */
static
jint JNICALL callback_all_alive_objects
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
	ResourcePrincipal* princ = (ResourcePrincipal*)user_data;

	// the class has a tag
	if ( (class_tag != (jlong)0)) {
		ClassDetails *d = (ClassDetails*)getDataFromTag(class_tag);
		
		char* className = getClassSignature(d);
		if (isClassClass(d)) {
			// it's a class object. I have to discover if I already visited it
			if ((*tag_ptr) == 0) return 0;			

        	d = (ClassDetails*)getDataFromTag(*tag_ptr);
			if (!isTagged(*tag_ptr)) {
				attachToPrincipal(*tag_ptr, princ);
				d = (ClassDetails*)getDataFromTag(class_tag);
        		d->count++;
        		d->space += (int)size;
				return JVMTI_VISIT_OBJECTS;
			}
			return 0;
		}
		else if (isTagged((*tag_ptr))) {
			// it is a tagged thread, or
			// it is an object already visited by this resource principal, or
			// it is an object already visited for another resource principal in this iteration
			return 0; // ignore it		
		}
		else if (!isTagged( (*tag_ptr))) {
			// It it neither a class object nor an object I already visited, so follow references and account of it
        	d = (ClassDetails*)getDataFromTag(class_tag);
        	d->count++;
        	d->space += (int)size;
			*tag_ptr = tagForObject(princ);	
			return JVMTI_VISIT_OBJECTS;
		}
		
    }
	return 0; // I don't know the class of this object, so don't explore it
}

/* Routine to explore all alive objects within the JVM */
static void
explore_FollowReferencesAll(
		jvmtiEnv* jvmti, ResourcePrincipal* principal)
{
	jvmtiError err;
	jvmtiHeapCallbacks heapCallbacks;

	(void)memset(&heapCallbacks, 0, sizeof(heapCallbacks));
    heapCallbacks.heap_reference_callback = &callback_all_alive_objects;
    err = (*jvmti)->FollowReferences(jvmti,
                   JVMTI_HEAP_FILTER_CLASS_UNTAGGED, NULL, NULL,
                   &heapCallbacks, (const void*)principal);
    check_jvmti_error(jvmti, err, "iterate through heap");
}

jint createPrincipal(jvmtiEnv* jvmti, 
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
		(*principals)[j].strategy_to_explore = &explore_FollowReferencesAll;
    }
	return count_principals;
}

/** Fill a structure with the infomation about the plugin 
* Returns 0 if everything was OK, a negative value otherwise	
*/
int DECLARE_FUNCTION(HeapAnalyzerPlugin* r)
{
	r->name = "all_alive_objects_exploration";
	r->description = "This plugin calculates the number of objects of each class within the whole JVM. It returns only alive objects";
	r->createPrincipals = &createPrincipal;
	return 0;
}
