#include "cJSON.h"

/* Implement RFC6901 (https://tools.ietf.org/html/rfc6901) JSON Pointer spec. */
cJSON *cJSONUtils_GetPointer(cJSON *object, const char *pointer);

/* Implement RFC6902 (https://tools.ietf.org/html/rfc6902) JSON Patch spec. */
cJSON* cJSONUtils_GeneratePatches(cJSON *from, cJSON *to);
/* Utility for generating patch array entries. */
void cJSONUtils_AddPatchToArray(cJSON *array, const char *op, const char *path, cJSON *val);
/* Returns 0 for success. */
int cJSONUtils_ApplyPatches(cJSON *object, cJSON *patches);

/*
// Note that ApplyPatches is NOT atomic on failure. To implement an atomic ApplyPatches, use:
//int cJSONUtils_AtomicApplyPatches(cJSON **object, cJSON *patches)
//{
//    cJSON *modme = cJSON_Duplicate(*object, 1);
//    int error = cJSONUtils_ApplyPatches(modme, patches);
//    if (!error)
//    {
//        cJSON_Delete(*object);
//        *object = modme;
//    }
//    else
//    {
//        cJSON_Delete(modme);
//    }
//
//    return error;
//}
// Code not added to library since this strategy is a LOT slower.
*/

/* Implement RFC7386 (https://tools.ietf.org/html/rfc7396) JSON Merge Patch spec. */
/* target will be modified by patch. return value is new ptr for target. */
cJSON* cJSONUtils_MergePatch(cJSON *target, cJSON *patch);
/* generates a patch to move from -> to */
cJSON *cJSONUtils_GenerateMergePatch(cJSON *from, cJSON *to);

/* Given a root object and a target object, construct a pointer from one to the other. */
char *cJSONUtils_FindPointerFromObjectTo(cJSON *object, cJSON *target);

/* Sorts the members of the object into alphabetical order. */
void cJSONUtils_SortObject(cJSON *object);
