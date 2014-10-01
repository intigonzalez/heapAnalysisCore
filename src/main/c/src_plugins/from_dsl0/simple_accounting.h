#ifndef __SIMPLE_ACCOUNTING__
#define __SIMPLE_ACCOUNTING__

// user-defined types

//this structure represents the data on each resource principal
typedef struct {
int nbObjects;
int nbSize;
} ResourcePrincipalData;

#include "common.h"
#include "RuntimeObjects.h"

extern ResourcePrincipalType TYPES[];

extern int nbTYPES;

#endif
