#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON_Utils.h"

// JSON Pointer implementation:
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

// JSON Patch implementation.
static void cJSONUtils_InplaceDecodePointerString(char *string)
{
	char *s2=string;
	for (;*string;s2++,string++)
		*s2=(*string!='~')?(*string):((*(++string)=='0')?'~':'/');
	*s2=0;
}

static cJSON *cJSONUtils_PatchDetach(cJSON *object,const char *path)
{
	char *parentptr=0,*childptr=0;cJSON *parent=0;

	parentptr=strdup(path);	childptr=strrchr(parentptr,'/');	if (childptr) *childptr++=0;
	parent=cJSONUtils_GetPointer(object,parentptr);
	cJSONUtils_InplaceDecodePointerString(childptr);

	cJSON *ret=0;
	if (!parent) ret=0;	// Couldn't find object to remove child from.
	else if (parent->type==cJSON_Array)		ret=cJSON_DetachItemFromArray(parent,atoi(childptr));
	else if (parent->type==cJSON_Object)	ret=cJSON_DetachItemFromObject(parent,childptr);
	free(parentptr);
	return ret;
}

static int cJSONUtils_ApplyPatch(cJSON *object,cJSON *patch)
{
	cJSON *op=0,*path=0,*value=0;int opcode=0;
	
	op=cJSON_GetObjectItem(patch,"op");
	path=cJSON_GetObjectItem(patch,"path");
	if (!op || !path) return 2;	// malformed patch.

	if		(!strcmp(op->valuestring,"add"))	opcode=0;
	else if (!strcmp(op->valuestring,"remove")) opcode=1;
	else if (!strcmp(op->valuestring,"replace"))opcode=2;
	else if (!strcmp(op->valuestring,"move"))	opcode=3;
	else if (!strcmp(op->valuestring,"copy"))	opcode=4;
	else if (!strcmp(op->valuestring,"test"))	opcode=5;
	else return 3; // unknown opcode.
	
	if (opcode==5) return 10; // TEST IS CURRENTLY UNIMPLEMENTED.

	if (opcode==1 || opcode==2)	// Remove/Replace
	{
		cJSON_Delete(cJSONUtils_PatchDetach(object,path->valuestring));	// Get rid of old.
		if (opcode==1) return 0;	// For Remove, this is job done.
	}

	if (opcode==3 || opcode==4)	// Copy/Move uses "from".
	{
		cJSON *from=cJSON_GetObjectItem(patch,"from");	if (!from) return 4; // missing "from" for copy/move.

		if (opcode==3) value=cJSONUtils_PatchDetach(object,from->valuestring);
		if (opcode==4) value=cJSONUtils_GetPointer(object,from->valuestring);
		if (!value) return 5; // missing "from" for copy/move.
		if (opcode==4) value=cJSON_Duplicate(value,1);
		if (!value) return 6; // out of memory for copy/move.
	}
	else	// Add/Replace uses "value".
	{
		value=cJSON_GetObjectItem(patch,"value");
		if (!value) return 7; // missing "value" for add/replace.
		value=cJSON_Duplicate(value,1);
		if (!value) return 8; // out of memory for add/replace.
	}
		
	// Now, just add "value" to "path".
	char *parentptr=0,*childptr=0;cJSON *parent=0;

	parentptr=strdup(path->valuestring);	childptr=strrchr(parentptr,'/');	if (childptr) *childptr++=0;
	parent=cJSONUtils_GetPointer(object,parentptr);
	cJSONUtils_InplaceDecodePointerString(childptr);

	// add, remove, replace, move, copy, test.
	if (!parent) {free(parentptr); return 9;}	// Couldn't find object to add to.
	else if (parent->type==cJSON_Array)
	{
		if (!strcmp(childptr,"-"))	cJSON_AddItemToArray(parent,value);
		else						cJSON_InsertItemInArray(parent,atoi(childptr),value);
	}
	else if (parent->type==cJSON_Object)
	{
		cJSON_DeleteItemFromObject(parent,childptr);
		cJSON_AddItemToObject(parent,childptr,value);
	}
	free(parentptr);
	return 0;
}


int cJSONUtils_ApplyPatches(cJSON *object,cJSON *patches)
{
	int err;
	if (!patches->type==cJSON_Array) return 1;	// malformed patches.
	if (patches) patches=patches->child;
	while (patches)
	{
		if ((err=cJSONUtils_ApplyPatch(object,patches))) return err;
		patches=patches->next;
	}
	return 0;
}

