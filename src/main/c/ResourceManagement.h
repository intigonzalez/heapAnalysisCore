
#ifndef __RESOURCE_PRINCIPAL__
#define __RESOURCE_PRINCIPAL__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jni.h"
#include "jvmti.h"

#define CLAZZ_CLAZZ 0x01
#define SYSTEM_CLAZZ 0x02

typedef struct {
	char* signature;
	jint flags;
//	jboolean is_clazz_clazz;
//	jboolean is_system_class;
} ClassInfo;

/* Typedef to hold class details */
typedef struct {
    int   count;
    int   space;
	ClassInfo* info;
} ClassDetails;

/* Typedef to hold a Resource Principal  */
typedef struct {
	char* name;
	jint tag;
	int count_details;
	ClassDetails* details;
	void* user_data;
	void* strategy_to_explore;
} ResourcePrincipal;

typedef struct {
	jint tag;
	jboolean followItsReferences;
	void* user_data;
} ObjectTag;


typedef void (*LocalExploration) 
	(jvmtiEnv* jvmti, ResourcePrincipal* principal);

typedef jint (*CreatePrincipals)
	(jvmtiEnv* jvmti, JNIEnv *jniEnv, ResourcePrincipal** principals, 
		ClassInfo* infos, int count_classes);

typedef jobject (*CreateResults)
	(jvmtiEnv*, JNIEnv *jniEnv, void* user_data);


/* Operations to get info from classes*/
char* getClassSignature(ClassDetails* d);
jboolean isClassClass(ClassDetails* d);
jboolean isSystemClass(ClassDetails* d);

/*Operation related to object tagging*/
jboolean isTagged(jlong t);
jboolean isTaggedByPrincipal(jlong t, ResourcePrincipal* p);
jint getLastTagCycleBoundary();
void increaseTagCycleBoundary(jint amount);
void removeTags(jvmtiEnv* jvmti);
jlong tagForObject(ResourcePrincipal* p);
jlong tagForObjectWithData(ResourcePrincipal* p, void* ud);
void* getDataFromTag(jlong tag);
void setUserDataForTag(jlong tag, void* ud);
void attachToPrincipal(jlong tag, ResourcePrincipal* p);

jboolean mustFollowReferences(jlong tag);
jboolean setFollowReferences(jlong tag, jboolean fr);

#endif
