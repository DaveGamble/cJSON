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

#define CJSON_PRINT_LIMIT 64

#include <string.h>

#include "unity/examples/unity_config.h"
#include "unity/src/unity.h"
#include "common.h"

/* a JSON string of n characters prints as n + 2 bytes, plus the terminating NUL */
static cJSON *create_string_of_length(size_t length, char character)
{
    char text[128];
    TEST_ASSERT_TRUE(length < sizeof(text));
    memset(text, character, length);
    text[length] = '\0';
    return cJSON_CreateString(text);
}

static void print_should_succeed_below_the_limit(void)
{
    /* 60 + 2 quotes = 62 bytes of output */
    cJSON *item = create_string_of_length(60, 'a');
    char *printed = cJSON_PrintUnformatted(item);
    TEST_ASSERT_NOT_NULL(printed);
    TEST_ASSERT_EQUAL_UINT(62, (unsigned int)strlen(printed));
    TEST_ASSERT_TRUE(strlen(printed) + 1 <= CJSON_PRINT_LIMIT);
    cJSON_free(printed);
    cJSON_Delete(item);
}

static void print_should_fail_above_the_limit(void)
{
    /* 62 + 2 quotes + NUL = 65 bytes, above the limit */
    cJSON *item = create_string_of_length(62, 'a');
    TEST_ASSERT_NULL(cJSON_PrintUnformatted(item));
    TEST_ASSERT_NULL(cJSON_Print(item));
    cJSON_Delete(item);
}

static void print_should_fail_when_escaping_exceeds_the_limit(void)
{
    /* 20 control characters escape to 20 * 6 + 2 = 122 bytes */
    cJSON *item = create_string_of_length(20, '\x01');
    TEST_ASSERT_NULL(cJSON_PrintUnformatted(item));
    TEST_ASSERT_NULL(cJSON_Print(item));
    TEST_ASSERT_NULL(cJSON_PrintBuffered(item, 1, false));
    cJSON_Delete(item);
}

static void print_preallocated_should_not_be_limited(void)
{
    char buffer[256];
    cJSON *item = create_string_of_length(20, '\x01');
    TEST_ASSERT_TRUE(cJSON_PrintPreallocated(item, buffer, (int)sizeof(buffer), false));
    TEST_ASSERT_EQUAL_UINT(122, (unsigned int)strlen(buffer));
    cJSON_Delete(item);
}

int CJSON_CDECL main(void)
{
    UNITY_BEGIN();

    RUN_TEST(print_should_succeed_below_the_limit);
    RUN_TEST(print_should_fail_above_the_limit);
    RUN_TEST(print_should_fail_when_escaping_exceeds_the_limit);
    RUN_TEST(print_preallocated_should_not_be_limited);

    return UNITY_END();
}
