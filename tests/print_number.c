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

static void assert_print_int(const char *expected, long long input)
{
    unsigned char printed[1024];
    unsigned char new_buffer[26];
    unsigned int i = 0;
    cJSON itemX[1];
    cJSON* item = itemX;
    printbuffer buffer = { 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.buffer = printed;
    buffer.length = sizeof(printed);
    buffer.offset = 0;
    buffer.noalloc = true;
    buffer.hooks = global_hooks;
    buffer.buffer = new_buffer;

    memset(itemX, 0, sizeof(itemX));
    memset(new_buffer, 0, sizeof(new_buffer));
    cJSON_SetIntValue(item, input);
    TEST_ASSERT_TRUE_MESSAGE(print_int(item, &buffer), "Failed to print number.");

    /* In MinGW or visual studio(before 2015),the exponten is represented using three digits,like:"1e-009","1e+017"
     * remove extra "0" to output "1e-09" or "1e+17",which makes testcase PASS */
    for(i = 0;i <sizeof(new_buffer);i++)
    {
        if(i >3 && new_buffer[i] =='0')
        {
            if((new_buffer[i-3] =='e' && new_buffer[i-2] == '-' && new_buffer[i] =='0') ||(new_buffer[i-2] =='e' && new_buffer[i-1] =='+'))
            {
                while(new_buffer[i] !='\0')
                {
                    new_buffer[i] = new_buffer[i+1];
                    i++;
                }
            }
        }
    }
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, buffer.buffer, "Printed number is not as expected.");
}

static void assert_print_float(const char *expected, double input)
{
    unsigned char printed[1024];
    unsigned char new_buffer[26];
    unsigned int i = 0;
    cJSON itemX[1];
    cJSON* item = itemX;
    printbuffer buffer = { 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.buffer = printed;
    buffer.length = sizeof(printed);
    buffer.offset = 0;
    buffer.noalloc = true;
    buffer.hooks = global_hooks;
    buffer.buffer = new_buffer;

    memset(itemX, 0, sizeof(itemX));
    memset(new_buffer, 0, sizeof(new_buffer));
    cJSON_SetFloatValue(item, input);
    TEST_ASSERT_TRUE_MESSAGE(print_float(item, &buffer), "Failed to print number.");

    /* In MinGW or visual studio(before 2015),the exponten is represented using three digits,like:"1e-009","1e+017"
     * remove extra "0" to output "1e-09" or "1e+17",which makes testcase PASS */
    for(i = 0;i <sizeof(new_buffer);i++)
    {
        if(i >3 && new_buffer[i] =='0')
        {
            if((new_buffer[i-3] =='e' && new_buffer[i-2] == '-' && new_buffer[i] =='0') ||(new_buffer[i-2] =='e' && new_buffer[i-1] =='+'))
            {
                while(new_buffer[i] !='\0')
                {
                    new_buffer[i] = new_buffer[i+1];
                    i++;
                }
            }
        }
    }
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, buffer.buffer, "Printed number is not as expected.");
}

static void print_number_should_print_zero(void)
{
    assert_print_int("0", 0);
    assert_print_float("0", 0);
}

static void print_number_should_print_negative_integers(void)
{
    assert_print_int("-1", -1.0);
    assert_print_int("-32768", -32768.0);
    assert_print_int("-2147483648", -2147483648.0);
    assert_print_float("-1", -1.0);
    assert_print_float("-32768", -32768.0);
    assert_print_float("-2147483648", -2147483648.0);
}

static void print_number_should_print_positive_integers(void)
{
    assert_print_int("1", 1.0);
    assert_print_int("32767", 32767.0);
    assert_print_int("2147483647", 2147483647.0);
    assert_print_float("1", 1.0);
    assert_print_float("32767", 32767.0);
    assert_print_float("2147483647", 2147483647.0);
}

static void print_number_should_print_positive_reals(void)
{
    assert_print_float("0.123", 0.123);
    assert_print_float("1e-09", 10e-10);
    assert_print_float("1000000000000", 10e11);
    assert_print_float("1.23e+129", 123e+127);
    assert_print_float("1.23e-126", 123e-128);
    assert_print_float("3.1415926535897931", 3.1415926535897931);
}

static void print_number_should_print_negative_reals(void)
{
    assert_print_float("-0.0123", -0.0123);
    assert_print_float("-1e-09", -10e-10);
    assert_print_float("-1e+21", -10e20);
    assert_print_float("-1.23e+129", -123e+127);
    assert_print_float("-1.23e-126", -123e-128);
}

static void print_number_should_print_non_number(void)
{
    TEST_IGNORE();
    /* FIXME: Cannot test this easily in C89! */
    /* assert_print_number("null", NaN); */
    /* assert_print_number("null", INFTY); */
    /* assert_print_number("null", -INFTY); */
}

int CJSON_CDECL main(void)
{
    /* initialize cJSON item */
    UNITY_BEGIN();

    RUN_TEST(print_number_should_print_zero);
    RUN_TEST(print_number_should_print_negative_integers);
    RUN_TEST(print_number_should_print_positive_integers);
    RUN_TEST(print_number_should_print_positive_reals);
    RUN_TEST(print_number_should_print_negative_reals);
    RUN_TEST(print_number_should_print_non_number);

    return UNITY_END();
}
