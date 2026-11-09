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
    int ok = 0;
    /* test all combinations */
    for (number = 0; number <= 0xFFFF; number++)
    {
        TEST_ASSERT_EQUAL_INT_MESSAGE(4, sprintf((char*)digits_lower, "%.4x", number), "sprintf failed.");
        TEST_ASSERT_EQUAL_INT_MESSAGE(4, sprintf((char*)digits_upper, "%.4X", number), "sprintf failed.");

        ok = 0;
        TEST_ASSERT_EQUAL_INT_MESSAGE(number, parse_hex4(digits_lower, &ok), "Failed to parse lowercase digits.");
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, ok, "parse_hex4 reported failure on valid lowercase digits.");
        ok = 0;
        TEST_ASSERT_EQUAL_INT_MESSAGE(number, parse_hex4(digits_upper, &ok), "Failed to parse uppercase digits.");
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, ok, "parse_hex4 reported failure on valid uppercase digits.");
    }
}

static void parse_hex4_should_parse_mixed_case(void)
{
    int ok = 0;
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"beef", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"beeF", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"beEf", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"beEF", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"bEef", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"bEeF", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"bEEf", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"bEEF", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"Beef", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BeeF", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BeEf", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BeEF", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BEef", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BEeF", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BEEf", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
    TEST_ASSERT_EQUAL_INT(0xBEEF, parse_hex4((const unsigned char*)"BEEF", &ok)); TEST_ASSERT_EQUAL_INT(1, ok);
}

static void parse_hex4_should_reject_invalid_digits(void)
{
    int ok = 0;
    TEST_ASSERT_EQUAL_INT(0, parse_hex4((const unsigned char*)"GGGG", &ok));
    TEST_ASSERT_EQUAL_INT(0, ok);
    ok = 0;
    TEST_ASSERT_EQUAL_INT(0, parse_hex4((const unsigned char*)"0G12", &ok));
    TEST_ASSERT_EQUAL_INT(0, ok);
    ok = 0;
    TEST_ASSERT_EQUAL_INT(0, parse_hex4((const unsigned char*)"----", &ok));
    TEST_ASSERT_EQUAL_INT(0, ok);
}

int CJSON_CDECL main(void)
{
    UNITY_BEGIN();
    RUN_TEST(parse_hex4_should_parse_all_combinations);
    RUN_TEST(parse_hex4_should_parse_mixed_case);
    RUN_TEST(parse_hex4_should_reject_invalid_digits);
    return UNITY_END();
}
