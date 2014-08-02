
#ifndef __PER_THREAD__
#define __PER_THREAD__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jni.h"
#include "jvmti.h"

#include "agent_util.h"
#include "ResourceManagement.h"


jint createPrincipal_per_thread(jvmtiEnv* jvmti, 
		ResourcePrincipal** principals, ClassInfo* infos, int count_classes);

#endif
