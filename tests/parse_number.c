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

static void assert_is_number(cJSON *number_item, cJSON_bool keep_string)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(number_item, "Item is NULL.");

    assert_not_in_list(number_item);
    assert_has_no_child(number_item);
    assert_has_type(number_item, cJSON_Number);
    assert_has_no_reference(number_item);
    assert_has_no_const_string(number_item);
    if (keep_string)
    {
        assert_has_valuestring(number_item);
    }
    else
    {
        assert_has_no_valuestring(number_item);
    }
    assert_has_no_string(number_item);
}

static void assert_parse_number_internal(const char *string, int integer, double real, cJSON_bool keep_string)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 }, false };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");
    buffer.hooks = global_hooks;
    buffer.keep_number_strings = keep_string;

    TEST_ASSERT_TRUE(parse_number(item, &buffer));
    assert_is_number(item, keep_string);
    TEST_ASSERT_EQUAL_INT(integer, item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(real, item->valuedouble);
    if (keep_string)
    {
        TEST_ASSERT_EQUAL_STRING_MESSAGE(string, item->valuestring, "The parsed result isn't as expected.");
    }
    else
    {
        TEST_ASSERT_EQUAL_STRING_MESSAGE(NULL, item->valuestring, "The string should be NULL.");
    }
    global_hooks.deallocate(item->valuestring);
    item->valuestring = NULL;
}

static void assert_parse_number(const char *string, int integer, double real)
{
    assert_parse_number_internal(string, integer, real, false);
    assert_parse_number_internal(string, integer, real, true);
}

static void assert_parse_number_failure(const char *string)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 }, false };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");
    buffer.hooks = global_hooks;

    TEST_ASSERT_FALSE(parse_number(item, &buffer));
}


static void parse_number_should_parse_zero(void)
{
    assert_parse_number("0", 0, 0);
    assert_parse_number("0.0", 0, 0.0);
    assert_parse_number("-0.0", 0, 0.0);
    assert_parse_number("-0", 0, -0.0);
}

static void parse_number_should_parse_negative_integers(void)
{
    assert_parse_number("-1", -1, -1);
    assert_parse_number("-32768", -32768, -32768.0);
    assert_parse_number("-2147483648", (int)-2147483648.0, -2147483648.0);
}

static void parse_number_should_parse_positive_integers(void)
{
    assert_parse_number("1", 1, 1);
    assert_parse_number("32767", 32767, 32767.0);
    assert_parse_number("2147483647", (int)2147483647.0, 2147483647.0);
}

static void parse_number_should_parse_positive_reals(void)
{
    assert_parse_number("0.001", 0, 0.001);
    assert_parse_number("10e-10", 0, 10e-10);
    assert_parse_number("10E-10", 0, 10e-10);
    assert_parse_number("10e10", INT_MAX, 10e10);
    assert_parse_number("123e+127", INT_MAX, 123e127);
    assert_parse_number("123e-128", 0, 123e-128);
}

static void parse_number_should_parse_negative_reals(void)
{
    assert_parse_number("-0.001", 0, -0.001);
    assert_parse_number("-10e-10", 0, -10e-10);
    assert_parse_number("-10E-10", 0, -10e-10);
    assert_parse_number("-10e20", INT_MIN, -10e20);
    assert_parse_number("-123e+127", INT_MIN, -123e127);
    assert_parse_number("-123e-128", 0, -123e-128);
}

static void parse_number_should_parse_large_number(void)
{
    assert_parse_number("12884901888", INT_MAX, 1.2884901888e10);
    assert_parse_number("100000000000000000000000000000000000000000000000000000000000000000.1",
                        INT_MAX,
                        99999999999999999209038626283633850822756121694230455365568299008.000000);
    assert_parse_number("1111111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000",
                        INT_MAX,
                        1111111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000.0);
    assert_parse_number("1e999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999",
                        INT_MAX,
                        (1.0/0.0));
    assert_parse_number("-1e999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999",
                        INT_MIN,
                        (-1.0/0.0));
    assert_parse_number("1e-999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999",
                        0,
                        0.0);
}

static void parse_number_invalid_checks(void)
{
    const char *tests[] = {
        "",
        "-",
        ".",
        ".0",
        "0.",
        "0.0.1",
        "e",
        "E",
        "e10",
        "E10",
        "0e",
        "0E",
        "1E+",
        "1E-",
        "1e1e2",
        "1E1E2",
        "09",
        "0.+",
    };
    size_t i = 0;

    for (i = 0; i < sizeof(tests)/sizeof(char *); i++)
    {
        assert_parse_number_failure(tests[i]);
    }
}

int CJSON_CDECL main(void)
{
    /* initialize cJSON item */
    memset(item, 0, sizeof(cJSON));
    UNITY_BEGIN();
    RUN_TEST(parse_number_should_parse_zero);
    RUN_TEST(parse_number_should_parse_negative_integers);
    RUN_TEST(parse_number_should_parse_positive_integers);
    RUN_TEST(parse_number_should_parse_positive_reals);
    RUN_TEST(parse_number_should_parse_negative_reals);
    RUN_TEST(parse_number_should_parse_large_number);
    RUN_TEST(parse_number_invalid_checks);
    return UNITY_END();
}
