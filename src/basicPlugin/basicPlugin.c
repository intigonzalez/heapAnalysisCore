
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../plugins.h"


/* create principals */
jint createPrincipal(jvmtiEnv* jvmti, 
		ResourcePrincipal** principals, ClassInfo* infos, int count_classes)
{
	(*principals) = (ResourcePrincipal*)(ResourcePrincipal*)calloc(sizeof(ResourcePrincipal), 0);
	printf("Creating the principals within basic plugin");
	return 0;
}

/** Fill a structure with the infomation about the plugin 
* Returns 0 if everything was OK, a negative value otherwise	
*/
int DECLARE_FUNCTION(HeapAnalyzerPlugin* r)
{
	r->name = "basicPlugin";
	r->description = "This plugin is used to demostrate the basic mechanism we use to implement plugins";
	r->createPrincipals = &createPrincipal;
	return 0;
}
