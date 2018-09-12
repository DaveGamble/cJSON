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

static cJSON item[1];

static void assert_is_string(cJSON *string_item)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(string_item, "Item is NULL.");

    assert_not_in_list(string_item);
    assert_has_no_child(string_item);
    assert_has_type(string_item, cJSON_String);
    assert_has_no_reference(string_item);
    assert_has_no_const_string(string_item);
    assert_has_valuestring(string_item);
    assert_has_no_string(string_item);
}

static void assert_parse_string(const char *string, const char *expected)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");
    buffer.hooks = global_hooks;

    TEST_ASSERT_TRUE_MESSAGE(parse_string(item, &buffer), "Couldn't parse string.");
    assert_is_string(item);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, item->valuestring, "The parsed result isn't as expected.");
    global_hooks.deallocate(item->valuestring);
    item->valuestring = NULL;
}

static void assert_not_parse_string(const char * const string)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");
    buffer.hooks = global_hooks;

    TEST_ASSERT_FALSE_MESSAGE(parse_string(item, &buffer), "Malformed string should not be accepted.");
    assert_is_invalid(item);
}



static void parse_string_should_parse_strings(void)
{
    assert_parse_string("\"\"", "");
    assert_parse_string(
        "\" !\\\"#$%&'()*+,-./\\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\\]^_'abcdefghijklmnopqrstuvwxyz{|}~\"",
        " !\"#$%&'()*+,-.//0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~");
    assert_parse_string(
        "\"\\\"\\\\\\/\\b\\f\\n\\r\\t\\u20AC\\u732b\"",
        "\"\\/\b\f\n\r\t€猫");
    reset(item);
    assert_parse_string("\"\b\f\n\r\t\"", "\b\f\n\r\t");
    reset(item);
}

static void parse_string_should_parse_utf16_surrogate_pairs(void)
{
    assert_parse_string("\"\\uD83D\\udc31\"", "🐱");
    reset(item);
}

static void parse_string_should_not_parse_non_strings(void)
{
    assert_not_parse_string("this\" is not a string\"");
    reset(item);
    assert_not_parse_string("");
    reset(item);
}

static void parse_string_should_not_parse_invalid_backslash(void)
{
    assert_not_parse_string("Abcdef\\123");
    reset(item);
    assert_not_parse_string("Abcdef\\e23");
    reset(item);
}

static void parse_string_should_not_overflow_with_closing_backslash(void)
{
    assert_not_parse_string("\"000000000000000000\\");
    reset(item);
}

static void parse_string_should_parse_bug_94(void)
{
    const char string[] = "\"~!@\\\\#$%^&*()\\\\\\\\-\\\\+{}[]:\\\\;\\\\\\\"\\\\<\\\\>?/.,DC=ad,DC=com\"";
    assert_parse_string(string, "~!@\\#$%^&*()\\\\-\\+{}[]:\\;\\\"\\<\\>?/.,DC=ad,DC=com");
    reset(item);
}

int CJSON_CDECL main(void)
{
    /* initialize cJSON item and error pointer */
    memset(item, 0, sizeof(cJSON));

    UNITY_BEGIN();
    RUN_TEST(parse_string_should_parse_strings);
    RUN_TEST(parse_string_should_parse_utf16_surrogate_pairs);
    RUN_TEST(parse_string_should_not_parse_non_strings);
    RUN_TEST(parse_string_should_not_parse_invalid_backslash);
    RUN_TEST(parse_string_should_parse_bug_94);
    RUN_TEST(parse_string_should_not_overflow_with_closing_backslash);
    return UNITY_END();
}
