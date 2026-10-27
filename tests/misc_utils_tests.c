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

static void apply_patches_should_leave_document_untouched_when_a_patch_fails(void)
{
    /* CVE-2026-67217: patch arrays must apply atomically. A failing
     * operation must not leave earlier operations applied. */
    cJSON *doc = cJSON_Parse("{\"a\": 1, \"b\": 2}");
    cJSON *patch = cJSON_Parse("[{\"op\": \"remove\", \"path\": \"/a\"}, "
                               "{\"op\": \"remove\", "
                               "\"path\": \"/nonexistent\"}]");
    cJSON *a = NULL;
    cJSON *b = NULL;

    TEST_ASSERT_NOT_NULL(doc);
    TEST_ASSERT_NOT_NULL(patch);

    TEST_ASSERT_NOT_EQUAL(0, cJSONUtils_ApplyPatches(doc, patch));

    a = cJSON_GetObjectItem(doc, "a");
    b = cJSON_GetObjectItem(doc, "b");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_INT(1, a->valueint);
    TEST_ASSERT_EQUAL_INT(2, b->valueint);

    cJSON_Delete(doc);
    cJSON_Delete(patch);
}

static void apply_patches_should_apply_valid_patches(void)
{
    cJSON *doc = cJSON_Parse("{\"a\": 1}");
    cJSON *patch = cJSON_Parse("[{\"op\": \"replace\", \"path\": \"/a\", "
                               "\"value\": 2}, "
                               "{\"op\": \"add\", \"path\": \"/c\", "
                               "\"value\": 3}]");
    cJSON *a = NULL;
    cJSON *c = NULL;

    TEST_ASSERT_NOT_NULL(doc);
    TEST_ASSERT_NOT_NULL(patch);

    TEST_ASSERT_EQUAL_INT(0, cJSONUtils_ApplyPatches(doc, patch));

    a = cJSON_GetObjectItem(doc, "a");
    c = cJSON_GetObjectItem(doc, "c");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL_INT(2, a->valueint);
    TEST_ASSERT_EQUAL_INT(3, c->valueint);

    cJSON_Delete(doc);
    cJSON_Delete(patch);
}

static void apply_patches_should_preserve_non_root_links(const cJSON_bool array, const cJSON_bool case_sensitive)
{
    int index = 0;
    for (index = 0; index < 3; index++)
    {
        cJSON *doc = cJSON_Parse(array ? "[{\"v\":1},{\"v\":1},{\"v\":1}]" :
                                "{\"first\":{\"v\":1},\"middle\":{\"v\":1},\"last\":{\"v\":1}}");
        cJSON *patch = cJSON_Parse("[{\"op\":\"replace\",\"path\":\"/v\",\"value\":2}]");
        cJSON *target = NULL;
        cJSON *previous = NULL;
        cJSON *next = NULL;

        TEST_ASSERT_NOT_NULL(doc);
        TEST_ASSERT_NOT_NULL(patch);
        target = cJSON_GetArrayItem(doc, index);
        TEST_ASSERT_NOT_NULL(target);
        previous = target->prev;
        next = target->next;

        TEST_ASSERT_EQUAL_INT(0, case_sensitive ? cJSONUtils_ApplyPatchesCaseSensitive(target, patch) :
                                                cJSONUtils_ApplyPatches(target, patch));
        TEST_ASSERT_EQUAL_PTR(previous, target->prev);
        TEST_ASSERT_EQUAL_PTR(next, target->next);
        TEST_ASSERT_EQUAL_PTR(target, cJSON_GetArrayItem(doc, index));
        TEST_ASSERT_EQUAL_PTR(index == 0 ? target : previous->next, target);
        if (next != NULL)
        {
            TEST_ASSERT_EQUAL_PTR(target, next->prev);
        }
        TEST_ASSERT_EQUAL_PTR(cJSON_GetArrayItem(doc, 2), doc->child->prev);
        TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(doc));
        TEST_ASSERT_EQUAL_INT(2, cJSON_GetObjectItem(target, "v")->valueint);
        if (!array)
        {
            TEST_ASSERT_EQUAL_STRING(index == 0 ? "first" : (index == 1 ? "middle" : "last"), target->string);
            TEST_ASSERT_EQUAL_PTR(target, cJSON_GetObjectItem(doc, target->string));
        }
        cJSON_Delete(patch);
        cJSON_Delete(doc);
    }
}

static void apply_patches_should_preserve_object_siblings(void)
{
    apply_patches_should_preserve_non_root_links(false, false);
    apply_patches_should_preserve_non_root_links(false, true);
}

static void apply_patches_should_preserve_array_siblings(void)
{
    apply_patches_should_preserve_non_root_links(true, false);
    apply_patches_should_preserve_non_root_links(true, true);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_utils_functions_shouldnt_crash_with_null_pointers);
    RUN_TEST(apply_patches_should_leave_document_untouched_when_a_patch_fails);
    RUN_TEST(apply_patches_should_apply_valid_patches);
    RUN_TEST(apply_patches_should_preserve_object_siblings);
    RUN_TEST(apply_patches_should_preserve_array_siblings);

    return UNITY_END();
}
