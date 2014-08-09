
#include "ResourceManagement.h"
#include "agent_util.h"


char* getClassSignature(ClassDetails* d)
{
	return d->info->signature;
}

jboolean isClassVisited(ClassDetails* d)
{
	return d->info->visited;
}

void setClassInfoVisited(ClassDetails* d, jboolean b)
{
	d->info->visited = b;
}

/* return true iff the className == Ljava/lang/Class; */
jboolean isClassClass(ClassDetails* d)
{
	//return !strcmp(d->info->signature, "Ljava/lang/Class;");
	return d->info->is_clazz_clazz;
}

/*Operation related to object tagging*/

static jint JNICALL
cbRemovingTag(jlong class_tag, jlong size, jlong* tag_ptr, jint length,
           void* user_data)
{
	//if ((*tag_ptr)) {
	(*tag_ptr) = (jlong)0;		
	//}
    return JVMTI_VISIT_OBJECTS;
}

void
removeTags(jvmtiEnv* jvmti)
{
	jvmtiError err;
	jvmtiHeapCallbacks heapCallbacks;
	(void)memset(&heapCallbacks, 0, sizeof(heapCallbacks));
    heapCallbacks.heap_iteration_callback = &cbRemovingTag;
    err = (*jvmti)->IterateThroughHeap(jvmti,
                    JVMTI_HEAP_FILTER_UNTAGGED, NULL,
	       	&heapCallbacks, NULL);
    check_jvmti_error(jvmti, err, "iterate through heap");
}

jboolean
isTagged(ResourcePrincipal* p, jlong t)
{
	return t != 0;
}

