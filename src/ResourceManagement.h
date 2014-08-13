
#ifndef __RESOURCE_PRINCIPAL__
#define __RESOURCE_PRINCIPAL__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jni.h"
#include "jvmti.h"

typedef struct {
	char* signature;
	jboolean is_clazz_clazz;
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
	jlong tag;
	int count_details;
	ClassDetails* details;
	void* strategy_to_explore;
} ResourcePrincipal;

typedef struct {
	jint tag;
	void* user_data;
} ObjectTag;


typedef void (*LocalExploration) 
	(jvmtiEnv* jvmti, ResourcePrincipal* principal);

typedef jint (*CreatePrincipals)
	(jvmtiEnv* jvmti, ResourcePrincipal** principals, 
		ClassInfo* infos, int count_classes);


/* Operations to get info from classes*/
char* getClassSignature(ClassDetails* d);
jboolean isClassClass(ClassDetails* d);

/*Operation related to object tagging*/
jboolean isTagged(jlong t);
jboolean isTaggedByPrincipal(jlong t, ResourcePrincipal* p);
void removeTags(jvmtiEnv* jvmti);
jlong tagForObject(ResourcePrincipal* p);
jlong tagForObjectWithData(ResourcePrincipal* p, void* ud);
void* getDataFromTag(jlong tag);
void setUserDataForTag(jlong tag, void* ud);
void attachToPrincipal(jlong tag, ResourcePrincipal* p);

#endif
