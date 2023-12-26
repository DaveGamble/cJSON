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

static void assert_is_number(cJSON *number_item)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(number_item, "Item is NULL.");

    assert_not_in_list(number_item);
    assert_has_no_child(number_item);
    assert_has_type(number_item, cJSON_Number);
    assert_has_no_reference(number_item);
    assert_has_no_const_string(number_item);
    assert_has_no_valuestring(number_item);
    assert_has_no_string(number_item);
}

static void assert_parse_number(const char *string, int integer, double real)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");

    TEST_ASSERT_TRUE(parse_number(item, &buffer));
    assert_is_number(item);
    TEST_ASSERT_EQUAL_INT(integer, item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(real, item->valuedouble);
}

#ifdef __CJSON_USE_INT64
static void assert_is_int64(cJSON *int64_number_item)
{
    assert_is_number(int64_number_item);
    TEST_ASSERT_BITS_MESSAGE(cJSON_IsInt64, cJSON_IsInt64, int64_number_item->type, "Item should be a int64 integer");
}

static void assert_parse_int64_number(const char *string, long long integer, double real)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = (const unsigned char*)string;
    buffer.length = strlen(string) + sizeof("");

    TEST_ASSERT_TRUE(parse_number(item, &buffer));
    TEST_ASSERT_EQUAL_INT64(integer, item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(real, item->valuedouble);
}

static void assert_parse_int64_number_with_type(const char *string, long long integer, double real)
{
    assert_parse_int64_number(string, integer, real);
    assert_is_int64(item);
}
#endif /* __CJSON_USE_INT64 */

static void parse_number_should_parse_zero(void)
{
    assert_parse_number("0", 0, 0);
    assert_parse_number("0.0", 0, 0.0);
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
#ifdef __CJSON_USE_INT64
    assert_parse_int64_number("10e10", 100000000000LL, 10e10);
    assert_parse_int64_number("123e+127", LLONG_MAX, 123e127);
#else
    assert_parse_number("10e10", INT_MAX, 10e10);
    assert_parse_number("123e+127", INT_MAX, 123e127);
#endif /* __CJSON_USE_INT64 */
    assert_parse_number("123e-128", 0, 123e-128);
}

static void parse_number_should_parse_negative_reals(void)
{
    assert_parse_number("-0.001", 0, -0.001);
    assert_parse_number("-10e-10", 0, -10e-10);
    assert_parse_number("-10E-10", 0, -10e-10);
#ifdef __CJSON_USE_INT64
    assert_parse_int64_number("-10e20", LLONG_MIN, -10e20);
    assert_parse_int64_number("-123e+127", LLONG_MIN, -123e127);
#else
    assert_parse_number("-10e20", INT_MIN, -10e20);
    assert_parse_number("-123e+127", INT_MIN, -123e127);
#endif /* __CJSON_USE_INT64 */
    assert_parse_number("-123e-128", 0, -123e-128);
}

#ifdef __CJSON_USE_INT64
static void parse_number_should_parse_int64_numbers(void)
{
    assert_parse_int64_number_with_type("0", 0LL, 0);
    assert_parse_int64_number_with_type("-1", -1LL, -1);
    assert_parse_int64_number_with_type("-32768", -32768LL, -32768.0);
    assert_parse_int64_number_with_type("-2147483648", -2147483648LL, -2147483648.0);
    assert_parse_int64_number_with_type("2147483648", (long long)INT_MAX + 1, 2147483648.0);
    assert_parse_int64_number_with_type("-2147483649", -2147483649LL, -2147483649.0);
    assert_parse_int64_number_with_type("9223372036854775807", LLONG_MAX, 9223372036854775807.0);
    assert_parse_int64_number_with_type("-9223372036854775808", LLONG_MIN, -9223372036854775808.0);
}
#endif /* __CJSON_USE_INT64 */

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
#ifdef __CJSON_USE_INT64
    RUN_TEST(parse_number_should_parse_int64_numbers);
#endif
    return UNITY_END();
}
