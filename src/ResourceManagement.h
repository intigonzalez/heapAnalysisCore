
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
	jboolean visited;
} ClassInfo;

/* Typedef to hold class details */
typedef struct {
    int   count;
    int   space;
	ClassInfo* info;
} ClassDetails;

/* Typedef to hold a Resource Principal  */
typedef struct {
	jlong tag;
	jlong previous_iteration_tag;
	int count_details;
	ClassDetails* details;
	void* strategy_to_explore;
} ResourcePrincipal;


typedef void (*LocalExploration) 
	(jvmtiEnv* jvmti, ResourcePrincipal* principal);

typedef jint (*CreatePrincipals)
	(jvmtiEnv* jvmti, ResourcePrincipal** principals, 
		ClassInfo* infos, int count_classes);


/* Operations to get info from classes*/
char* getClassSignature(ClassDetails* d);
jboolean isClassVisited(ClassDetails* d);
void setClassInfoVisited(ClassDetails* d, jboolean b);
jboolean isClassClass(ClassDetails* d);

/*Operation related to object tagging*/
void removeTags(jvmtiEnv* jvmti);

/* Operation to handle an infinite sequence */
jlong getLastInSequence();
jlong nextInSequence();

#endif
