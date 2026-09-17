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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity/examples/unity_config.h"
#include "unity/src/unity.h"
#include "common.h"
#include "../cJSON_Utils.h"

static void cjson_utils_functions_shouldnt_crash_with_null_pointers(void)
{
    cJSON *item = cJSON_CreateString("item");
    TEST_ASSERT_NOT_NULL(item);

    TEST_ASSERT_NULL(cJSONUtils_GetPointer(item, NULL));
    TEST_ASSERT_NULL(cJSONUtils_GetPointer(NULL, "pointer"));
    TEST_ASSERT_NULL(cJSONUtils_GetPointerCaseSensitive(NULL, "pointer"));
    TEST_ASSERT_NULL(cJSONUtils_GetPointerCaseSensitive(item, NULL));
    TEST_ASSERT_NULL(cJSONUtils_GeneratePatches(item, NULL));
    TEST_ASSERT_NULL(cJSONUtils_GeneratePatches(NULL, item));
    TEST_ASSERT_NULL(cJSONUtils_GeneratePatchesCaseSensitive(item, NULL));
    TEST_ASSERT_NULL(cJSONUtils_GeneratePatchesCaseSensitive(NULL, item));
    cJSONUtils_AddPatchToArray(item, "path", "add", NULL);
    cJSONUtils_AddPatchToArray(item, "path", NULL, item);
    cJSONUtils_AddPatchToArray(item, NULL, "add", item);
    cJSONUtils_AddPatchToArray(NULL, "path", "add", item);
    cJSONUtils_ApplyPatches(item, NULL);
    cJSONUtils_ApplyPatches(NULL, item);
    cJSONUtils_ApplyPatchesCaseSensitive(item, NULL);
    cJSONUtils_ApplyPatchesCaseSensitive(NULL, item);
    TEST_ASSERT_NULL(cJSONUtils_MergePatch(item, NULL));
    item = cJSON_CreateString("item");
    TEST_ASSERT_NULL(cJSONUtils_MergePatchCaseSensitive(item, NULL));
    item = cJSON_CreateString("item");
    /* these calls are actually valid */
    /* cJSONUtils_MergePatch(NULL, item); */
    /* cJSONUtils_MergePatchCaseSensitive(NULL, item);*/
    /* cJSONUtils_GenerateMergePatch(item, NULL); */
    /* cJSONUtils_GenerateMergePatch(NULL, item); */
    /* cJSONUtils_GenerateMergePatchCaseSensitive(item, NULL); */
    /* cJSONUtils_GenerateMergePatchCaseSensitive(NULL, item); */

    TEST_ASSERT_NULL(cJSONUtils_FindPointerFromObjectTo(item, NULL));
    TEST_ASSERT_NULL(cJSONUtils_FindPointerFromObjectTo(NULL, item));
    cJSONUtils_SortObject(NULL);
    cJSONUtils_SortObjectCaseSensitive(NULL);

    cJSON_Delete(item);
}

static void cjson_utils_apply_patches_should_reject_null_valuestring(void)
{
    /* A cJSON_String item can have a NULL valuestring (e.g. when created with
     * cJSON_CreateStringReference(NULL)). The JSON patch code checks the "op",
     * "path" and "from" fields with cJSON_IsString() and then dereferences
     * valuestring. Such patches must be rejected with an error code instead of
     * crashing on the NULL valuestring. */

    /* NULL "op" valuestring */
    {
        cJSON *object = cJSON_CreateObject();
        cJSON *patches = cJSON_CreateArray();
        cJSON *patch = cJSON_CreateObject();
        cJSON_AddItemToObject(patch, "op", cJSON_CreateStringReference(NULL));
        cJSON_AddItemToObject(patch, "path", cJSON_CreateString(""));
        cJSON_AddItemToObject(patch, "value", cJSON_CreateNumber(1));
        cJSON_AddItemToArray(patches, patch);
        TEST_ASSERT_TRUE_MESSAGE(0 != cJSONUtils_ApplyPatches(object, patches), "NULL \"op\" valuestring should be rejected.");
        TEST_ASSERT_TRUE_MESSAGE(0 != cJSONUtils_ApplyPatchesCaseSensitive(object, patches), "NULL \"op\" valuestring should be rejected.");
        cJSON_Delete(object);
        cJSON_Delete(patches);
    }

    /* NULL "path" valuestring */
    {
        cJSON *object = cJSON_CreateObject();
        cJSON *patches = cJSON_CreateArray();
        cJSON *patch = cJSON_CreateObject();
        cJSON_AddItemToObject(patch, "op", cJSON_CreateString("remove"));
        cJSON_AddItemToObject(patch, "path", cJSON_CreateStringReference(NULL));
        cJSON_AddItemToArray(patches, patch);
        TEST_ASSERT_TRUE_MESSAGE(0 != cJSONUtils_ApplyPatches(object, patches), "NULL \"path\" valuestring should be rejected.");
        TEST_ASSERT_TRUE_MESSAGE(0 != cJSONUtils_ApplyPatchesCaseSensitive(object, patches), "NULL \"path\" valuestring should be rejected.");
        cJSON_Delete(object);
        cJSON_Delete(patches);
    }

    /* NULL "from" valuestring */
    {
        cJSON *object = cJSON_CreateObject();
        cJSON *patches = cJSON_CreateArray();
        cJSON *patch = cJSON_CreateObject();
        cJSON_AddItemToObject(object, "a", cJSON_CreateNumber(1));
        cJSON_AddItemToObject(patch, "op", cJSON_CreateString("move"));
        cJSON_AddItemToObject(patch, "path", cJSON_CreateString("/b"));
        cJSON_AddItemToObject(patch, "from", cJSON_CreateStringReference(NULL));
        cJSON_AddItemToArray(patches, patch);
        TEST_ASSERT_TRUE_MESSAGE(0 != cJSONUtils_ApplyPatches(object, patches), "NULL \"from\" valuestring should be rejected.");
        TEST_ASSERT_TRUE_MESSAGE(0 != cJSONUtils_ApplyPatchesCaseSensitive(object, patches), "NULL \"from\" valuestring should be rejected.");
        cJSON_Delete(object);
        cJSON_Delete(patches);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_utils_functions_shouldnt_crash_with_null_pointers);
    RUN_TEST(cjson_utils_apply_patches_should_reject_null_valuestring);

    return UNITY_END();
}
