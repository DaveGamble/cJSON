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

static void parse_hex4_should_parse_all_combinations(void)
{
    unsigned int number = 0;
    unsigned char digits_lower[6];
    unsigned char digits_upper[6];
    /* test all combinations */
    for (number = 0; number <= 0xFFFF; number++)
    {
        TEST_ASSERT_EQUAL_INT_MESSAGE(4, sprintf((char*)digits_lower, "%.4x", number), "sprintf failed.");
        TEST_ASSERT_EQUAL_INT_MESSAGE(4, sprintf((char*)digits_upper, "%.4X", number), "sprintf failed.");

        TEST_ASSERT_EQUAL_INT_MESSAGE(number, parse_hex4(digits_lower), "Failed to parse lowercase digits.");
        TEST_ASSERT_EQUAL_INT_MESSAGE(number, parse_hex4(digits_upper), "Failed to parse uppercase digits.");
    }
}

static void parse_hex4_should_parse_mixed_case(void)
{
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"beef"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"beeF"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"beEf"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"beEF"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"bEef"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"bEeF"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"bEEf"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"bEEF"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"Beef"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BeeF"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BeEf"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BeEF"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BEef"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BEeF"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BEEf"));
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BEEF"));
}

int CJSON_CDECL main(void)
{
    UNITY_BEGIN();
    RUN_TEST(parse_hex4_should_parse_all_combinations);
    RUN_TEST(parse_hex4_should_parse_mixed_case);
    return UNITY_END();
}
