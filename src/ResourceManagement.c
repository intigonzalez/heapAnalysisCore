
#include "ResourceManagement.h"


typedef struct {
	jlong n;
} Sequence;

Sequence globalSeq = {.n = 1};

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


jlong getLastInSequence()
{
	return globalSeq.n;
}

jlong nextInSequence()
{
	return ++globalSeq.n;
}

