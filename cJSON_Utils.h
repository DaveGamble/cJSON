#include "cJSON.h"

// Implement RFC6901 (https://tools.ietf.org/html/rfc6901) JSON Pointer spec.
cJSON *cJSONUtils_GetPointer(cJSON *object,const char *pointer);

// Implement RFC6902 (https://tools.ietf.org/html/rfc6902) JSON Patch spec.
//cJSON* cJSONUtils_GeneratePatches(cJSON *from,cJSON *to);	// Not yet implemented.
int cJSONUtils_ApplyPatches(cJSON *object,cJSON *patches);	// Returns 0 for success.

// Note that ApplyPatches is NOT atomic on failure. To implement an atomic ApplyPatches, use:
//int cJSONUtils_AtomicApplyPatches(cJSON **object, cJSON *patches)
//{
//	cJSON *modme=cJSON_Duplicate(*object,1);
//	int error=cJSONUtils_ApplyPatches(modme,patches);
//	if (!error)	{cJSON_Delete(*object);*object=modme;}
//	else		cJSON_Delete(modme);
//	return error;
//}
// Code not added to library since this strategy is a LOT slower.
