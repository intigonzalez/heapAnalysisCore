#include "list.h"
#include "simple_accounting.h"

// methods to initialize properties

List*
all_root_objects(EntityEnvironment* env,
		void* ud)
{
	ResourcePrincipalData* princ = 
		(ResourcePrincipalData*)ud;
	return threads;
}
int
all_membership(LocalEnvironment* env, 
		void* ud)
{
	ResourcePrincipalData* princ = 
		(ResourcePrincipalData*)ud;
	int tmp0 = belongs_to(THIS,NONE);
	return tmp0;
}
void
all_on_inclusion(LocalEnvironment* env, 
		void* ud)
{
	ResourcePrincipalData* princ = 
		(ResourcePrincipalData*)ud;
	int tmp1 = princ->nbObjects + 1;
	princ->nbObjects = tmp1;
	int tmp2 = THIS->size;
	int tmp3 = princ->nbSize + tmp2;
	princ->nbSize = tmp3;
}
int
all_nbObjects(EntityEnvironment* env,
		void* ud)
{
	ResourcePrincipalData* princ = 
		(ResourcePrincipalData*)ud;
	return 0;
}
int
all_nbSize(EntityEnvironment* env,
		void* ud)
{
	ResourcePrincipalData* princ = 
		(ResourcePrincipalData*)ud;
	return 0;
}

// methods to obtain instances' names
char*
all_init_names(GlobalEnvironment* env)
{
	return "all-jvm";
}

// routines to initialize user-defined functions

void all_initialize
	(EntityEnvironment* env, 
	void* ud)
{
	ResourcePrincipalData* princ =
			(ResourcePrincipalData*)ud;
	princ->nbObjects = all_nbObjects(env,princ);
	princ->nbSize = all_nbSize(env,princ);
}

// method to create new instances of ResourcePrincipalData
void* instances_createResourcePrincipalData()
{
	return calloc(sizeof(ResourcePrincipalData), 1);
}

// ResourcePrincipalTypes
ResourcePrincipalType TYPES[] = {
{ 
  instances_createResourcePrincipalData,
  all_init_names, 
  0, 
  all_root_objects,
  all_membership,
  all_on_inclusion,
  all_initialize
}
};

int nbTYPES = 1;
