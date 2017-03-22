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

#include "unity/examples/unity_config.h"
#include "unity/src/unity.h"
#include "common.h"

static void parse_with_opts_should_handle_null(void)
{
    const char *error_pointer = NULL;
    cJSON *item = NULL;
    TEST_ASSERT_NULL_MESSAGE(cJSON_ParseWithOpts(NULL, &error_pointer, false), "Failed to handle NULL input.");
    item = cJSON_ParseWithOpts("{}", NULL, false);
    TEST_ASSERT_NOT_NULL_MESSAGE(item, "Failed to handle NULL error pointer.");
    cJSON_Delete(item);
    TEST_ASSERT_NULL_MESSAGE(cJSON_ParseWithOpts(NULL, NULL, false), "Failed to handle both NULL.");
    TEST_ASSERT_NULL_MESSAGE(cJSON_ParseWithOpts("{", NULL, false), "Failed to handle NULL error pointer with parse error.");
}

static void parse_with_opts_should_handle_empty_strings(void)
{
    const char empty_string[] = "";
    const char *error_pointer = NULL;
    TEST_ASSERT_NULL(cJSON_ParseWithOpts(empty_string, NULL, false));
    error_pointer = cJSON_GetErrorPtr();
    TEST_ASSERT_EQUAL_INT(0, error_pointer - empty_string);
    TEST_ASSERT_NULL(cJSON_ParseWithOpts(empty_string, &error_pointer, false));
    TEST_ASSERT_EQUAL_INT(0, error_pointer - empty_string);
}

static void parse_with_opts_should_require_null_if_requested(void)
{
    cJSON *item = cJSON_ParseWithOpts("{}", NULL, true);
    TEST_ASSERT_NOT_NULL(item);
    cJSON_Delete(item);
    item = cJSON_ParseWithOpts("{} \n", NULL, true);
    TEST_ASSERT_NOT_NULL(item);
    cJSON_Delete(item);
    TEST_ASSERT_NULL(cJSON_ParseWithOpts("{}x", NULL, true));
}

static void parse_with_opts_should_return_parse_end(void)
{
    const char json[] = "[] empty array XD";
    const char *parse_end = NULL;

    cJSON *item = cJSON_ParseWithOpts(json, &parse_end, false);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(2, parse_end - json);
    cJSON_Delete(item);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(parse_with_opts_should_handle_null);
    RUN_TEST(parse_with_opts_should_handle_empty_strings);
    RUN_TEST(parse_with_opts_should_require_null_if_requested);
    RUN_TEST(parse_with_opts_should_return_parse_end);

    return UNITY_END();
}
