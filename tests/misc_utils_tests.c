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

static void apply_root_patch(cJSON *target, const char *patch_json)
{
    cJSON *patch = cJSON_Parse(patch_json);
    TEST_ASSERT_NOT_NULL(patch);
    TEST_ASSERT_EQUAL_INT(0, cJSONUtils_ApplyPatchesCaseSensitive(target, patch));
    cJSON_Delete(patch);
}

static void cjson_utils_replace_root_should_not_free_referenced_child(void)
{
    cJSON *shared = cJSON_Parse("{\"k\":\"v\"}");
    cJSON *reference = cJSON_CreateObjectReference(shared);
    TEST_ASSERT_NOT_NULL(reference);

    apply_root_patch(reference, "[{\"op\":\"replace\",\"path\":\"\",\"value\":1}]");
    TEST_ASSERT_TRUE(cJSON_IsNumber(reference));
    TEST_ASSERT_EQUAL_STRING("v", cJSON_GetObjectItem(shared, "k")->valuestring);

    cJSON_Delete(reference);
    cJSON_Delete(shared);
}

static void cjson_utils_remove_root_should_not_free_referenced_child(void)
{
    cJSON *shared = cJSON_Parse("[1,2]");
    cJSON *reference = cJSON_CreateArrayReference(shared);
    TEST_ASSERT_NOT_NULL(reference);

    apply_root_patch(reference, "[{\"op\":\"remove\",\"path\":\"\"}]");
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(shared));

    cJSON_Delete(reference);
    cJSON_Delete(shared);
}

static void cjson_utils_replace_root_should_not_free_referenced_valuestring(void)
{
    static char owned_by_caller[] = "owned";
    cJSON *reference = cJSON_CreateStringReference(owned_by_caller);
    TEST_ASSERT_NOT_NULL(reference);

    apply_root_patch(reference, "[{\"op\":\"replace\",\"path\":\"\",\"value\":1}]");
    TEST_ASSERT_TRUE(cJSON_IsNumber(reference));
    TEST_ASSERT_EQUAL_STRING("owned", owned_by_caller);

    cJSON_Delete(reference);
}

static void cjson_utils_replace_root_should_not_free_constant_key(void)
{
    cJSON *container = cJSON_CreateObject();
    cJSON *member = NULL;
    cJSON_AddItemToObjectCS(container, "cfg", cJSON_CreateString("x"));
    member = cJSON_GetObjectItem(container, "cfg");
    TEST_ASSERT_NOT_NULL(member);

    apply_root_patch(member, "[{\"op\":\"replace\",\"path\":\"\",\"value\":2}]");
    TEST_ASSERT_TRUE(cJSON_IsNumber(member));

    cJSON_Delete(container);
}

static void cjson_utils_replace_root_should_not_free_constant_value_key(void)
{
    cJSON *document = cJSON_Parse("{\"x\":1}");
    cJSON *patches = cJSON_CreateArray();
    cJSON *operation = cJSON_CreateObject();
    cJSON_AddItemToObject(operation, "op", cJSON_CreateString("replace"));
    cJSON_AddItemToObject(operation, "path", cJSON_CreateString(""));
    cJSON_AddItemToObjectCS(operation, "value", cJSON_CreateNumber(1));
    cJSON_AddItemToArray(patches, operation);

    TEST_ASSERT_EQUAL_INT(0, cJSONUtils_ApplyPatchesCaseSensitive(document, patches));
    TEST_ASSERT_TRUE(cJSON_IsNumber(document));
    TEST_ASSERT_NULL(document->string);
    TEST_ASSERT_FALSE(document->type & cJSON_StringIsConst);

    cJSON_Delete(patches);
    cJSON_Delete(document);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_utils_functions_shouldnt_crash_with_null_pointers);
    RUN_TEST(cjson_utils_replace_root_should_not_free_referenced_child);
    RUN_TEST(cjson_utils_remove_root_should_not_free_referenced_child);
    RUN_TEST(cjson_utils_replace_root_should_not_free_referenced_valuestring);
    RUN_TEST(cjson_utils_replace_root_should_not_free_constant_key);
    RUN_TEST(cjson_utils_replace_root_should_not_free_constant_value_key);

    return UNITY_END();
}
