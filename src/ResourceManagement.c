
#include "ResourceManagement.h"
#include "agent_util.h"


char* getClassSignature(ClassDetails* d)
{
	return d->info->signature;
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
	if ((*tag_ptr)) {
		free((void*)(ptrdiff_t)(*tag_ptr));
		(*tag_ptr) = (jlong)0;		
	}
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
isTagged(jlong t)
{
	static int x = 0;
	//return t != 0;
	ObjectTag* tag;
	if (t == 0) return JNI_FALSE;
	tag = (ObjectTag*)(void*)(ptrdiff_t)t;
	return tag->tag != 0; // tag!=0 && tag->tag != 0; hence, !(tag!=0 && tag->tag != 0) the same as (tag == 0 || tag->tag == 0)
}

jboolean
isTaggedByPrincipal(jlong t, ResourcePrincipal* p)
{
	ObjectTag* tag;
	if (t == 0) return JNI_FALSE;
	tag = (ObjectTag*)(void*)(ptrdiff_t)t;
	return tag->tag == p->tag;
}

jlong
tagForObject(ResourcePrincipal* p)
{
	ObjectTag* t = (ObjectTag*)calloc(sizeof(ObjectTag),1);
	if (p) {
		t->tag = p->tag;	
	}
	return (jlong)t;
}

jlong
tagForObjectWithData(ResourcePrincipal* p, void* ud)
{
	ObjectTag* t;
	jlong l = tagForObject(p);
	t = (ObjectTag*)(void*)(ptrdiff_t)l;
	t->user_data = ud;
	return l;
}

void* getDataFromTag(jlong tag)
{
	ObjectTag* t = (ObjectTag*)(void*)(ptrdiff_t)tag;
	return t->user_data;
}

void
setUserDataForTag(jlong tag, void* ud)
{
	ObjectTag* t = (ObjectTag*)(void*)(ptrdiff_t)tag;
	t->user_data = ud;
}

void
attachToPrincipal(jlong tag, ResourcePrincipal* p)
{
	ObjectTag* t = (ObjectTag*)(void*)(ptrdiff_t)tag;
	t->tag = p->tag;
}

