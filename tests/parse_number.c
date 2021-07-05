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

static void assert_is_int(cJSON *number_item)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(number_item, "Item is NULL.");

    assert_not_in_list(number_item);
    assert_has_no_child(number_item);
    assert_has_type(number_item, cJSON_Int);
    assert_has_no_reference(number_item);
    assert_has_no_const_string(number_item);
    assert_has_no_valuestring(number_item);
    assert_has_no_string(number_item);
}

static void assert_is_float(cJSON *number_item)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(number_item, "Item is NULL.");

    assert_not_in_list(number_item);
    assert_has_no_child(number_item);
    assert_has_type(number_item, cJSON_Float);
    assert_has_no_reference(number_item);
    assert_has_no_const_string(number_item);
    assert_has_no_valuestring(number_item);
    assert_has_no_string(number_item);
}

static void assert_parse_int_fail(const char *string) {
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");

    cJSON_bool b = parse_number(item, &buffer);
    TEST_ASSERT_FALSE(b);
    reset(item);
}

static void assert_parse_int(const char *string, int64_t integer)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");

    cJSON_bool b = parse_number(item, &buffer);
    TEST_ASSERT_TRUE(b);
    assert_is_int(item);
    TEST_ASSERT_EQUAL_INT(integer, item->valueint);
}

// static void assert_parse_float_fail(const char *string)
// {
//     parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
//     buffer.content = (const unsigned char*)string;
//     buffer.length = strlen(string) + sizeof("");
//
//     cJSON_bool b = parse_number(item, &buffer);
//     TEST_ASSERT_FALSE(b);
// }

static void assert_parse_float(const char *string, double real)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");

    cJSON_bool b = parse_number(item, &buffer);
    TEST_ASSERT_TRUE(b);
    assert_is_float(item);
    TEST_ASSERT_EQUAL_DOUBLE(real, item->valuedouble);
}

static void parse_number_should_parse_zero(void)
{
    assert_parse_int("0", 0);
    assert_parse_int("-0", 0);
    assert_parse_float("0.0", 0.0);
    assert_parse_float("-0.0", -0.0);
}

static void parse_number_fail(void)
{
    assert_parse_int_fail("-9223372036854775809");
    assert_parse_int_fail("9223372036854775808");
}

static void parse_number_should_parse_negative_integers(void)
{
    assert_parse_int("-1", -1);
    assert_parse_int("-32768", -32768);
    assert_parse_int("-2147483648", -2147483648);
    assert_parse_int("-9223372036854775808", INT64_MIN);
    assert_parse_float("-1.0", -1.0);
    assert_parse_float("-32768.0", -32768.0);
    assert_parse_float("-2147483648.0", -2147483648.0);
}

static void parse_number_should_parse_positive_integers(void)
{
    assert_parse_int("1", 1);
    assert_parse_int("32767", 32767);
    assert_parse_int("2147483647", 2147483647);
    assert_parse_int("9223372036854775807", INT64_MAX);
    assert_parse_float("1.0", 1);
    assert_parse_float("32767.0", 32767.0);
    assert_parse_float("2147483647.0", 2147483647.0);
}

static void parse_number_should_parse_positive_reals(void)
{
    assert_parse_float("0.001", 0.001);
    assert_parse_float("10e-10", 10e-10);
    assert_parse_float("10E-10", 10e-10);
    assert_parse_float("10e10", 10e10);
    assert_parse_float("123e+127", 123e127);
    assert_parse_float("123e-128", 123e-128);
}

static void parse_number_should_parse_negative_reals(void)
{
    assert_parse_float("-0.001", -0.001);
    assert_parse_float("-10e-10", -10e-10);
    assert_parse_float("-10E-10", -10e-10);
    assert_parse_float("-10e20", -10e20);
    assert_parse_float("-123e+127", -123e127);
    assert_parse_float("-123e-128", -123e-128);
}

int CJSON_CDECL main(void)
{
    /* initialize cJSON item */
    memset(item, 0, sizeof(cJSON));
    UNITY_BEGIN();
    RUN_TEST(parse_number_should_parse_zero);
    RUN_TEST(parse_number_fail);
    RUN_TEST(parse_number_should_parse_negative_integers);
    RUN_TEST(parse_number_should_parse_positive_integers);
    RUN_TEST(parse_number_should_parse_positive_reals);
    RUN_TEST(parse_number_should_parse_negative_reals);
    return UNITY_END();
}
