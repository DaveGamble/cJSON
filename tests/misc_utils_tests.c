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

/* Build {"lvl": {"lvl": ... {"end": 1} ... }} nested levels deep. */
static cJSON *build_nested_object(int levels)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *current = root;
    int i = 0;

    if (root == NULL)
    {
        return NULL;
    }
    for (i = 0; i < levels; i++)
    {
        cJSON *next = cJSON_CreateObject();
        if (next == NULL)
        {
            cJSON_Delete(root);
            return NULL;
        }
        cJSON_AddItemToObject(current, "lvl", next);
        current = next;
    }
    cJSON_AddNumberToObject(current, "end", 1);
    return root;
}

static void apply_patches_should_reject_grafts_past_the_nesting_limit(void)
{
    /* CVE-2026-67215: add/copy must not amplify the document past the
     * nesting limit (single copy grafting a 600-deep subtree under a
     * depth-600 parent would reach depth 1202). */
    cJSON *inner = build_nested_object(600);
    cJSON *doc = cJSON_CreateObject();
    cJSON *patch = cJSON_CreateArray();
    cJSON *op = cJSON_CreateObject();
    char path[8192] = "/d";
    int i = 0;

    TEST_ASSERT_NOT_NULL(inner);
    TEST_ASSERT_NOT_NULL(doc);
    TEST_ASSERT_NOT_NULL(patch);
    TEST_ASSERT_NOT_NULL(op);
    cJSON_AddItemToObject(doc, "d", inner);
    for (i = 0; i < 599; i++)
    {
        strcat(path, "/lvl");
    }
    strcat(path, "/copy");
    cJSON_AddStringToObject(op, "op", "copy");
    cJSON_AddStringToObject(op, "from", "/d");
    cJSON_AddStringToObject(op, "path", path);
    cJSON_AddItemToArray(patch, op);

    TEST_ASSERT_NOT_EQUAL(0, cJSONUtils_ApplyPatches(doc, patch));

    cJSON_Delete(doc);
    cJSON_Delete(patch);
}

static void apply_patches_should_allow_grafts_within_the_nesting_limit(void)
{
    cJSON *doc = cJSON_Parse("{\"a\": {\"b\": 1}}");
    cJSON *patch = cJSON_Parse("[{\"op\": \"copy\", \"from\": \"/a\", "
                               "\"path\": \"/c\"}]");

    TEST_ASSERT_NOT_NULL(doc);
    TEST_ASSERT_NOT_NULL(patch);
    TEST_ASSERT_EQUAL_INT(0, cJSONUtils_ApplyPatches(doc, patch));
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(doc, "c"));

    cJSON_Delete(doc);
    cJSON_Delete(patch);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_utils_functions_shouldnt_crash_with_null_pointers);
    RUN_TEST(apply_patches_should_reject_grafts_past_the_nesting_limit);
    RUN_TEST(apply_patches_should_allow_grafts_within_the_nesting_limit);

    return UNITY_END();
}
