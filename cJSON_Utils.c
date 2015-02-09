#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON_Utils.h"

cJSON *cJSONUtils_GetPointer(cJSON *object,const char *pointer)
{
	cJSON *target=object;int which=0;const char *s=0;int len=0;char *element=0,*e=0;
	
	while (*pointer=='/' && object)
	{
		pointer++;
		if (object->type==cJSON_Array)
		{
			which=0;
			while (*pointer>='0' && *pointer<='9') {which*=10;which+=*pointer++ - '0';}
			if (*pointer && *pointer!='/') return 0;
			object=cJSON_GetArrayItem(object,which);
		}
		else if (object->type==cJSON_Object)
		{
		
			s=pointer;len=0;
			while (*s && *s!='/') {if (*s!='~') len++; s++;}
			e=element=malloc(len+1); if (!element) return 0;
			element[len]=0;

			while (*pointer && *pointer!='/')
			{
				if (*pointer=='~' && pointer[1]=='0')		*e++='~',pointer+=2;
				else if (*pointer=='~' && pointer[1]=='1')	*e++='/',pointer+=2;
				else if (*pointer=='~')						{free(element); return 0;}	// Invalid encoding.
				else										*e++=*pointer++; 
			}
			object=cJSON_GetObjectItem(object,element);
			free(element);
		}
		else return 0;
	}
	return object;
}


