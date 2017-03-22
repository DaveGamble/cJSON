/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "cJSON.h"

/* Implement RFC6901 (https://tools.ietf.org/html/rfc6901) JSON Pointer spec. */
CJSON_PUBLIC(cJSON *) cJSONUtils_GetPointer(cJSON *object, const char *pointer);

/* Implement RFC6902 (https://tools.ietf.org/html/rfc6902) JSON Patch spec. */
CJSON_PUBLIC(cJSON *) cJSONUtils_GeneratePatches(cJSON *from, cJSON *to);
/* Utility for generating patch array entries. */
CJSON_PUBLIC(void) cJSONUtils_AddPatchToArray(cJSON *array, const char *op, const char *path, cJSON *val);
/* Returns 0 for success. */
CJSON_PUBLIC(int) cJSONUtils_ApplyPatches(cJSON *object, cJSON *patches);

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
CJSON_PUBLIC(cJSON *) cJSONUtils_MergePatch(cJSON *target, cJSON *patch);
/* generates a patch to move from -> to */
CJSON_PUBLIC(cJSON *) cJSONUtils_GenerateMergePatch(cJSON *from, cJSON *to);

/* Given a root object and a target object, construct a pointer from one to the other. */
CJSON_PUBLIC(char *) cJSONUtils_FindPointerFromObjectTo(cJSON *object, cJSON *target);

/* Sorts the members of the object into alphabetical order. */
CJSON_PUBLIC(void) cJSONUtils_SortObject(cJSON *object);
