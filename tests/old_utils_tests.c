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

/* JSON Apply Merge tests: */
static const char *merges[15][3] =
{
    {"{\"a\":\"b\"}", "{\"a\":\"c\"}", "{\"a\":\"c\"}"},
    {"{\"a\":\"b\"}", "{\"b\":\"c\"}", "{\"a\":\"b\",\"b\":\"c\"}"},
    {"{\"a\":\"b\"}", "{\"a\":null}", "{}"},
    {"{\"a\":\"b\",\"b\":\"c\"}", "{\"a\":null}", "{\"b\":\"c\"}"},
    {"{\"a\":[\"b\"]}", "{\"a\":\"c\"}", "{\"a\":\"c\"}"},
    {"{\"a\":\"c\"}", "{\"a\":[\"b\"]}", "{\"a\":[\"b\"]}"},
    {"{\"a\":{\"b\":\"c\"}}", "{\"a\":{\"b\":\"d\",\"c\":null}}", "{\"a\":{\"b\":\"d\"}}"},
    {"{\"a\":[{\"b\":\"c\"}]}", "{\"a\":[1]}", "{\"a\":[1]}"},
    {"[\"a\",\"b\"]", "[\"c\",\"d\"]", "[\"c\",\"d\"]"},
    {"{\"a\":\"b\"}", "[\"c\"]", "[\"c\"]"},
    {"{\"a\":\"foo\"}", "null", "null"},
    {"{\"a\":\"foo\"}", "\"bar\"", "\"bar\""},
    {"{\"e\":null}", "{\"a\":1}", "{\"e\":null,\"a\":1}"},
    {"[1,2]", "{\"a\":\"b\",\"c\":null}", "{\"a\":\"b\"}"},
    {"{}","{\"a\":{\"bb\":{\"ccc\":null}}}", "{\"a\":{\"bb\":{}}}"}
};

static void json_pointer_tests(void)
{
    cJSON *root = NULL;
    const char *json=
        "{"
        "\"foo\": [\"bar\", \"baz\"],"
        "\"\": 0,"
        "\"a/b\": 1,"
        "\"c%d\": 2,"
        "\"e^f\": 3,"
        "\"g|h\": 4,"
        "\"i\\\\j\": 5,"
        "\"k\\\"l\": 6,"
        "\" \": 7,"
        "\"m~n\": 8"
        "}";

    root = cJSON_Parse(json);

    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, ""), root);
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/foo"), cJSON_GetObjectItem(root, "foo"));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/foo/0"), cJSON_GetObjectItem(root, "foo")->child);
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/foo/0"), cJSON_GetObjectItem(root, "foo")->child);
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/"), cJSON_GetObjectItem(root, ""));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/a~1b"), cJSON_GetObjectItem(root, "a/b"));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/c%d"), cJSON_GetObjectItem(root, "c%d"));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/c^f"), cJSON_GetObjectItem(root, "c^f"));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/c|f"), cJSON_GetObjectItem(root, "c|f"));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/i\\j"), cJSON_GetObjectItem(root, "i\\j"));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/k\"l"), cJSON_GetObjectItem(root, "k\"l"));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/ "), cJSON_GetObjectItem(root, " "));
    TEST_ASSERT_EQUAL_PTR(cJSONUtils_GetPointer(root, "/m~0n"), cJSON_GetObjectItem(root, "m~n"));

    cJSON_Delete(root);
}

static void misc_tests(void)
{
    /* Misc tests */
    int numbers[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    cJSON *object = NULL;
    cJSON *object1 = NULL;
    cJSON *object2 = NULL;
    cJSON *object3 = NULL;
    cJSON *object4 = NULL;
    cJSON *nums = NULL;
    cJSON *num6 = NULL;
    char *pointer = NULL;

    printf("JSON Pointer construct\n");
    object = cJSON_CreateObject();
    nums = cJSON_CreateIntArray(numbers, 10);
    num6 = cJSON_GetArrayItem(nums, 6);
    cJSON_AddItemToObject(object, "numbers", nums);

    pointer = cJSONUtils_FindPointerFromObjectTo(object, num6);
    TEST_ASSERT_EQUAL_STRING("/numbers/6", pointer);
    free(pointer);

    pointer = cJSONUtils_FindPointerFromObjectTo(object, nums);
    TEST_ASSERT_EQUAL_STRING("/numbers", pointer);
    free(pointer);

    pointer = cJSONUtils_FindPointerFromObjectTo(object, object);
    TEST_ASSERT_EQUAL_STRING("", pointer);
    free(pointer);

    object1 = cJSON_CreateObject();
    object2 = cJSON_CreateString("m~n");
    cJSON_AddItemToObject(object1, "m~n", object2);
    pointer = cJSONUtils_FindPointerFromObjectTo(object1, object2);
    TEST_ASSERT_EQUAL_STRING("/m~0n",pointer);
    free(pointer);

    object3 = cJSON_CreateObject();
    object4 = cJSON_CreateString("m/n");
    cJSON_AddItemToObject(object3, "m/n", object4);
    pointer = cJSONUtils_FindPointerFromObjectTo(object3, object4);
    TEST_ASSERT_EQUAL_STRING("/m~1n",pointer);
    free(pointer);

    cJSON_Delete(object);
    cJSON_Delete(object1);
    cJSON_Delete(object3);
}

static void sort_tests(void)
{
    /* Misc tests */
    const char *random = "QWERTYUIOPASDFGHJKLZXCVBNM";
    char buf[2] = {'\0', '\0'};
    cJSON *sortme = NULL;
    size_t i = 0;
    cJSON *current_element = NULL;

    /* JSON Sort test: */
    sortme = cJSON_CreateObject();
    for (i = 0; i < 26; i++)
    {
        buf[0] = random[i];
        cJSON_AddItemToObject(sortme, buf, cJSON_CreateNumber(1));
    }

    cJSONUtils_SortObject(sortme);

    /* check sorting */
    current_element = sortme->child->next;
    for (i = 1; (i < 26) && (current_element != NULL) && (current_element->prev != NULL); i++)
    {
        TEST_ASSERT_TRUE(current_element->string[0] >= current_element->prev->string[0]);
        current_element = current_element->next;
    }

    cJSON_Delete(sortme);
}

static void merge_tests(void)
{
    size_t i = 0;
    char *patchtext = NULL;
    char *after = NULL;

    /* Merge tests: */
    printf("JSON Merge Patch tests\n");
    for (i = 0; i < 15; i++)
    {
        cJSON *object_to_be_merged = cJSON_Parse(merges[i][0]);
        cJSON *patch = cJSON_Parse(merges[i][1]);
        patchtext = cJSON_PrintUnformatted(patch);
        object_to_be_merged = cJSONUtils_MergePatch(object_to_be_merged, patch);
        after = cJSON_PrintUnformatted(object_to_be_merged);
        TEST_ASSERT_EQUAL_STRING(merges[i][2], after);

        free(patchtext);
        free(after);
        cJSON_Delete(object_to_be_merged);
        cJSON_Delete(patch);
    }
}

static void generate_merge_tests(void)
{
    size_t i = 0;
    char *patchedtext = NULL;

    /* Generate Merge tests: */
    for (i = 0; i < 15; i++)
    {
        cJSON *from = cJSON_Parse(merges[i][0]);
        cJSON *to = cJSON_Parse(merges[i][2]);
        cJSON *patch = cJSONUtils_GenerateMergePatch(from,to);
        from = cJSONUtils_MergePatch(from,patch);
        patchedtext = cJSON_PrintUnformatted(from);
        TEST_ASSERT_EQUAL_STRING(merges[i][2], patchedtext);

        cJSON_Delete(from);
        cJSON_Delete(to);
        cJSON_Delete(patch);
        free(patchedtext);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(json_pointer_tests);
    RUN_TEST(misc_tests);
    RUN_TEST(sort_tests);
    RUN_TEST(merge_tests);
    RUN_TEST(generate_merge_tests);

    return UNITY_END();
}
