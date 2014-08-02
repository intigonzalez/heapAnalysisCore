#ifndef __ALL_OBJECTS__
#define __ALL_OBJECTS__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jni.h"
#include "jvmti.h"

#include "agent_util.h"
#include "ResourceManagement.h"

jint create_single_principal(jvmtiEnv* jvmti, 
		ResourcePrincipal** principals,ClassInfo* infos, int count_classes);


#endif
