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

static void assert_print_string(const char *expected, const char *input)
{
    unsigned char printed[1024];
    printbuffer buffer = { 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.buffer = printed;
    buffer.length = sizeof(printed);
    buffer.offset = 0;
    buffer.noalloc = true;
    buffer.hooks = global_hooks;

    TEST_ASSERT_TRUE_MESSAGE(print_string_ptr((const unsigned char*)input, &buffer), "Failed to print string.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, printed, "The printed string isn't as expected.");
}

static void print_string_should_print_empty_strings(void)
{
    assert_print_string("\"\"", "");
    assert_print_string("\"\"", NULL);
}

static void print_string_should_print_ascii(void)
{
    char ascii[0x7F];
    size_t i = 1;

    /* create ascii table */
    for (i = 1; i < 0x7F; i++)
    {
        ascii[i-1] = (char)i;
    }
    ascii[0x7F-1] = '\0';

    assert_print_string("\"\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\b\\t\\n\\u000b\\f\\r\\u000e\\u000f\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f !\\\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\"",
            ascii);
}

static void print_string_should_print_utf8(void)
{
    assert_print_string("\"ü猫慕\"", "ü猫慕");
}

int CJSON_CDECL main(void)
{
    /* initialize cJSON item */
    UNITY_BEGIN();

    RUN_TEST(print_string_should_print_empty_strings);
    RUN_TEST(print_string_should_print_ascii);
    RUN_TEST(print_string_should_print_utf8);

    return UNITY_END();
}
