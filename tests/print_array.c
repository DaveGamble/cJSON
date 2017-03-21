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

static void assert_print_array(const char * const expected, const char * const input)
{
    unsigned char printed_unformatted[1024];
    unsigned char printed_formatted[1024];

    const unsigned char *error_pointer;
    cJSON item[1];

    printbuffer formatted_buffer;
    printbuffer unformatted_buffer;

    parse_buffer parsebuffer;
    parsebuffer.content = (const unsigned char*)input;
    parsebuffer.length = strlen(input) + sizeof("");
    parsebuffer.offset = 0;

    /* buffer for formatted printing */
    formatted_buffer.buffer = printed_formatted;
    formatted_buffer.length = sizeof(printed_formatted);
    formatted_buffer.offset = 0;
    formatted_buffer.noalloc = true;

    /* buffer for unformatted printing */
    unformatted_buffer.buffer = printed_unformatted;
    unformatted_buffer.length = sizeof(printed_unformatted);
    unformatted_buffer.offset = 0;
    unformatted_buffer.noalloc = true;

    memset(item, 0, sizeof(item));
    TEST_ASSERT_TRUE_MESSAGE(parse_array(item, &parsebuffer, &error_pointer, &global_hooks), "Failed to parse array.");

    TEST_ASSERT_TRUE_MESSAGE(print_array(item, 0, false, &unformatted_buffer, &global_hooks), "Failed to print unformatted string.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(input, printed_unformatted, "Unformatted array is not correct.");

    TEST_ASSERT_TRUE_MESSAGE(print_array(item, 0, true, &formatted_buffer, &global_hooks), "Failed to print formatted string.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, printed_formatted, "Formatted array is not correct.");

    reset(item);
}

static void print_array_should_print_empty_arrays(void)
{
    assert_print_array("[]", "[]");
}

static void print_array_should_print_arrays_with_one_element(void)
{

    assert_print_array("[1]", "[1]");
    assert_print_array("[\"hello!\"]", "[\"hello!\"]");
    assert_print_array("[[]]", "[[]]");
    assert_print_array("[null]", "[null]");
}

static void print_array_should_print_arrays_with_multiple_elements(void)
{
    assert_print_array("[1, 2, 3]", "[1,2,3]");
    assert_print_array("[1, null, true, false, [], \"hello\", {\n\t}]", "[1,null,true,false,[],\"hello\",{}]");
}

int main(void)
{
    /* initialize cJSON item */
    UNITY_BEGIN();

    RUN_TEST(print_array_should_print_empty_arrays);
    RUN_TEST(print_array_should_print_arrays_with_one_element);
    RUN_TEST(print_array_should_print_arrays_with_multiple_elements);

    return UNITY_END();
}
