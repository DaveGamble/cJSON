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

static cJSON *parse_test_file(const char * const filename)
{
    char *file = NULL;
    cJSON *json = NULL;

    file = read_file(filename);
    TEST_ASSERT_NOT_NULL_MESSAGE(file, "Failed to read file.");

    json = cJSON_Parse(file);
    TEST_ASSERT_NOT_NULL_MESSAGE(json, "Failed to parse test json.");
    TEST_ASSERT_TRUE_MESSAGE(cJSON_IsArray(json), "Json is not an array.");

    free(file);

    return json;
}

static cJSON_bool test_apply_patch(const cJSON * const test)
{
    cJSON *doc = NULL;
    cJSON *patch = NULL;
    cJSON *expected = NULL;
    cJSON *error_element = NULL;
    cJSON *comment = NULL;
    cJSON *disabled = NULL;

    cJSON *object = NULL;
    cJSON_bool successful = false;

    /* extract all the data out of the test */
    comment = cJSON_GetObjectItemCaseSensitive(test, "comment");
    if (cJSON_IsString(comment))
    {
        printf("Testing \"%s\"\n", comment->valuestring);
    }
    else
    {
        printf("Testing unknown\n");
    }

    disabled = cJSON_GetObjectItemCaseSensitive(test, "disabled");
    if (cJSON_IsTrue(disabled))
    {
        printf("SKIPPED\n");
        return true;
    }

    doc = cJSON_GetObjectItemCaseSensitive(test, "doc");
    TEST_ASSERT_NOT_NULL_MESSAGE(doc, "No \"doc\" in the test.");
    patch = cJSON_GetObjectItemCaseSensitive(test, "patch");
    TEST_ASSERT_NOT_NULL_MESSAGE(patch, "No \"patch\"in the test.");
    /* Make a working copy of 'doc' */
    object = cJSON_Duplicate(doc, true);
    TEST_ASSERT_NOT_NULL(object);

    expected = cJSON_GetObjectItemCaseSensitive(test, "expected");
    error_element = cJSON_GetObjectItemCaseSensitive(test, "error");
    if (error_element != NULL)
    {
        /* excepting an error */
        TEST_ASSERT_TRUE_MESSAGE(0 != cJSONUtils_ApplyPatchesCaseSensitive(object, patch), "Test didn't fail as it's supposed to.");

        successful = true;
    }
    else
    {
        /* apply the patch */
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, cJSONUtils_ApplyPatchesCaseSensitive(object, patch), "Failed to apply patches.");
        successful = true;

        if (expected != NULL)
        {
            successful = cJSON_Compare(object, expected, true);
        }
    }

    cJSON_Delete(object);

    if (successful)
    {
        printf("OK\n");
    }
    else
    {
        printf("FAILED\n");
    }

    return successful;
}

static cJSON_bool test_generate_test(cJSON *test)
{
    cJSON *doc = NULL;
    cJSON *patch = NULL;
    cJSON *expected = NULL;
    cJSON *disabled = NULL;

    cJSON *object = NULL;
    cJSON_bool successful = false;

    char *printed_patch = NULL;

    disabled = cJSON_GetObjectItemCaseSensitive(test, "disabled");
    if (cJSON_IsTrue(disabled))
    {
        printf("SKIPPED\n");
        return true;
    }

    doc = cJSON_GetObjectItemCaseSensitive(test, "doc");
    TEST_ASSERT_NOT_NULL_MESSAGE(doc, "No \"doc\" in the test.");

    /* Make a working copy of 'doc' */
    object = cJSON_Duplicate(doc, true);
    TEST_ASSERT_NOT_NULL(object);

    expected = cJSON_GetObjectItemCaseSensitive(test, "expected");
    if (expected == NULL)
    {
        cJSON_Delete(object);
        /* if there is no expected output, this test doesn't make sense */
        return true;
    }

    patch = cJSONUtils_GeneratePatchesCaseSensitive(doc, expected);
    TEST_ASSERT_NOT_NULL_MESSAGE(patch, "Failed to generate patches.");

    printed_patch = cJSON_Print(patch);
    printf("%s\n", printed_patch);
    free(printed_patch);

    /* apply the generated patch */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, cJSONUtils_ApplyPatchesCaseSensitive(object, patch), "Failed to apply generated patch.");

    successful = cJSON_Compare(object, expected, true);

    cJSON_Delete(patch);
    cJSON_Delete(object);

    if (successful)
    {
        printf("generated patch: OK\n");
    }
    else
    {
        printf("generated patch: FAILED\n");
    }

    return successful;
}

static void cjson_utils_should_pass_json_patch_test_tests(void)
{
    cJSON *tests = parse_test_file("json-patch-tests/tests.json");
    cJSON *test = NULL;

    cJSON_bool failed = false;
    cJSON_ArrayForEach(test, tests)
    {
        failed |= !test_apply_patch(test);
        failed |= !test_generate_test(test);
    }

    cJSON_Delete(tests);

    TEST_ASSERT_FALSE_MESSAGE(failed, "Some tests failed.");
}

static void cjson_utils_should_pass_json_patch_test_spec_tests(void)
{
    cJSON *tests = parse_test_file("json-patch-tests/spec_tests.json");
    cJSON *test = NULL;

    cJSON_bool failed = false;
    cJSON_ArrayForEach(test, tests)
    {
        failed |= !test_apply_patch(test);
        failed |= !test_generate_test(test);
    }

    cJSON_Delete(tests);

    TEST_ASSERT_FALSE_MESSAGE(failed, "Some tests failed.");
}

static void cjson_utils_should_pass_json_patch_test_cjson_utils_tests(void)
{
    cJSON *tests = parse_test_file("json-patch-tests/cjson-utils-tests.json");
    cJSON *test = NULL;

    cJSON_bool failed = false;
    cJSON_ArrayForEach(test, tests)
    {
        failed |= !test_apply_patch(test);
        failed |= !test_generate_test(test);
    }

    cJSON_Delete(tests);

    TEST_ASSERT_FALSE_MESSAGE(failed, "Some tests failed.");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_utils_should_pass_json_patch_test_tests);
    RUN_TEST(cjson_utils_should_pass_json_patch_test_spec_tests);
    RUN_TEST(cjson_utils_should_pass_json_patch_test_cjson_utils_tests);

    return UNITY_END();
}
