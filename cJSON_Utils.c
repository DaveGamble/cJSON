#include <ctype.h>
#include "cJSON_Utils.h"

static int cJSONUtils_Pstrcasecmp(const char *a,const char *e)
{
	if (!a || !e) return (a==e)?0:1;
	for (;*a && *e && *e!='/';a++,e++) {
		if (*e=='~') {if (!(e[1]=='0' && *a=='~') && !(e[1]=='1' && *a=='/')) return 1;  else e++;}
		else if (tolower(*a)!=tolower(*e)) return 1;
	}
	if ((*e!=0 && *e!='/') != (*a!=0)) return 1;
	return 0;
}


cJSON *cJSONUtils_GetPointer(cJSON *object,const char *pointer)
{
	cJSON *target=object;int which=0;const char *element=0;
	
	while (*pointer=='/' && object)
	{
		pointer++;
		if (object->type==cJSON_Array)
		{
			which=0; while (*pointer>='0' && *pointer<='9') which=(10*which) + *pointer++ - '0';
			if (*pointer && *pointer!='/') return 0;
			object=cJSON_GetArrayItem(object,which);
		}
		else if (object->type==cJSON_Object)
		{
			element=pointer; while (*pointer && *pointer!='/') pointer++;
			object=object->child;	while (object && cJSONUtils_Pstrcasecmp(object->string,element)) object=object->next;	// GetObjectItem.
		}
		else return 0;
	}
	return object;
}


