#ifndef __HEAP_ANALYZER_PLUGIN__
#define __HEAP_ANALYZER_PLUGIN__

#include "ResourceManagement.h"

typedef struct {
	char* name;
	char* description;
	CreatePrincipals createPrincipals;
} HeapAnalyzerPlugin;

#define DECLARE_FUNCTION declarePlugin

typedef int (*PluginDeclareFunction)(HeapAnalyzerPlugin* r);

#endif
