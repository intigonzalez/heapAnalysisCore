
#include "ResourceManagement.h"
#include "agent_util.h"


#define MAX_NUMBER_OF_TAGS ((4096 + 1024)*1024)
static jint tagCycle = 0;
static ObjectTag tagRegion[MAX_NUMBER_OF_TAGS];
static ObjectTag* currentTag = tagRegion;
static ObjectTag* lastAddress = &(tagRegion[MAX_NUMBER_OF_TAGS]);

char* getClassSignature(ClassDetails* d)
{
	return d->info->signature;
}

/* return true iff the className == Ljava/lang/Class; */
jboolean
isClassClass(ClassDetails* d)
{
	//return !strcmp(d->info->signature, "Ljava/lang/Class;");
	return (d->info->flags & CLAZZ_CLAZZ) != 0;
}

jboolean
isSystemClass(ClassDetails* d)
{
    return (d->info->flags & SYSTEM_CLAZZ) != 0;
}

/*Operation related to object tagging*/

static jint JNICALL
cbRemovingTag(jlong class_tag, jlong size, jlong* tag_ptr, jint length,
           void* user_data)
{
	if ((*tag_ptr)) {
//		free((void*)(ptrdiff_t)(*tag_ptr));
		(*tag_ptr) = (jlong)0;
	}
    return JVMTI_VISIT_OBJECTS;
}

void
removeTags(jvmtiEnv* jvmti)
{
//	jvmtiError err;
//	jvmtiHeapCallbacks heapCallbacks;
//	(void)memset(&heapCallbacks, 0, sizeof(heapCallbacks));
//    heapCallbacks.heap_iteration_callback = &cbRemovingTag;
//    err = (*jvmti)->IterateThroughHeap(jvmti,
//                    JVMTI_HEAP_FILTER_UNTAGGED, NULL,
//	       	&heapCallbacks, NULL);
//    check_jvmti_error(jvmti, err, "iterate through heap");
    currentTag = tagRegion;
}

jboolean
isTagged(jlong t)
{
	static int x = 0;
	//return t != 0;
	ObjectTag* tag;
	if (t == 0) return JNI_FALSE;
	tag = (ObjectTag*)(void*)(ptrdiff_t)t;

	return tag->tag != 0 && tag < currentTag;
	//return tag->tag != 0; // tag!=0 && tag->tag != 0; hence, !(tag!=0 && tag->tag != 0) the same as (tag == 0 || tag->tag == 0)
}

jboolean
isTaggedByPrincipal(jlong t, ResourcePrincipal* p)
{
	ObjectTag* tag;
	if (t == 0) return JNI_FALSE;
	tag = (ObjectTag*)(void*)(ptrdiff_t)t;
	return tag->tag == p->tag && tag < currentTag;
	//return tag->tag == p->tag;
}

jint
getLastTagCycleBoundary()
{
    //return tagCycle;
    return 0;
}

void
increaseTagCycleBoundary(jint amount)
{
//    tagCycle += amount;
}

jlong
tagForObject(ResourcePrincipal* p)
{
//	ObjectTag* t = (ObjectTag*)calloc(sizeof(ObjectTag),1);
	ObjectTag* t = currentTag;
    t->user_data = 0;
    t->tag = 0;
	if (p) {
		t->tag = p->tag;
	}
	currentTag++;
	if (currentTag == lastAddress) {
	    fprintf(stderr, "================== Error creating objects =====================\n");
	    exit(1);
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

jboolean
mustFollowReferences(jlong tag)
{
	ObjectTag* t = (ObjectTag*)(void*)(ptrdiff_t)tag;
	return t->followItsReferences;
}

jboolean
setFollowReferences(jlong tag, jboolean fr)
{
	ObjectTag* t = (ObjectTag*)(void*)(ptrdiff_t)tag;
	t->followItsReferences = fr;
}

