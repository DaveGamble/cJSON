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

static const unsigned char *error_pointer = NULL;

static void assert_is_array(cJSON *array_item)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(array_item, "Item is NULL.");

    assert_not_in_list(array_item);
    assert_has_type(array_item, cJSON_Array);
    assert_has_no_reference(array_item);
    assert_has_no_const_string(array_item);
    assert_has_no_valuestring(array_item);
    assert_has_no_string(array_item);
}

static void assert_not_array(const char *json)
{
    TEST_ASSERT_NULL(parse_array(item, (const unsigned char*)json, &error_pointer));
    assert_is_invalid(item);
}

static void assert_parse_array(const char *json)
{
    TEST_ASSERT_NOT_NULL(parse_array(item, (const unsigned char*)json, &error_pointer));
    assert_is_array(item);
}

static void parse_array_should_parse_empty_arrays(void)
{
    assert_parse_array("[]");
    assert_has_no_child(item);

    assert_parse_array("[\n\t]");
    assert_has_no_child(item);
}


static void parse_array_should_parse_arrays_with_one_element(void)
{

    assert_parse_array("[1]");
    assert_has_child(item);
    assert_has_type(item->child, cJSON_Number);
    reset(item);

    assert_parse_array("[\"hello!\"]");
    assert_has_child(item);
    assert_has_type(item->child, cJSON_String);
    TEST_ASSERT_EQUAL_STRING("hello!", item->child->valuestring);
    reset(item);

    assert_parse_array("[[]]");
    assert_has_child(item);
    assert_is_array(item->child);
    assert_has_no_child(item->child);
    reset(item);

    assert_parse_array("[null]");
    assert_has_child(item);
    assert_has_type(item->child, cJSON_NULL);
    reset(item);
}

static void parse_array_should_parse_arrays_with_multiple_elements(void)
{
    assert_parse_array("[1\t,\n2, 3]");
    assert_has_child(item);
    TEST_ASSERT_NOT_NULL(item->child->next);
    TEST_ASSERT_NOT_NULL(item->child->next->next);
    TEST_ASSERT_NULL(item->child->next->next->next);
    assert_has_type(item->child, cJSON_Number);
    assert_has_type(item->child->next, cJSON_Number);
    assert_has_type(item->child->next->next, cJSON_Number);
    reset(item);

    {
        size_t i = 0;
        cJSON *node = NULL;
        int expected_types[7] =
        {
            cJSON_Number,
            cJSON_NULL,
            cJSON_True,
            cJSON_False,
            cJSON_Array,
            cJSON_String,
            cJSON_Object
        };
        assert_parse_array("[1, null, true, false, [], \"hello\", {}]");

        node = item->child;
        for (
                i = 0;
                (i < (sizeof(expected_types)/sizeof(int)))
                && (node != NULL);
                i++, node = node->next)
        {
            TEST_ASSERT_BITS(0xFF, expected_types[i], node->type);
        }
        TEST_ASSERT_EQUAL_INT(i, 7);
        reset(item);
    }
}

static void parse_array_should_not_parse_non_arrays(void)
{
    assert_not_array("");
    assert_not_array("[");
    assert_not_array("]");
    assert_not_array("{\"hello\":[]}");
    assert_not_array("42");
    assert_not_array("3.14");
    assert_not_array("\"[]hello world!\n\"");
}

int main(void)
{
    /* initialize cJSON item */
    memset(item, 0, sizeof(cJSON));

    UNITY_BEGIN();
    RUN_TEST(parse_array_should_parse_empty_arrays);
    RUN_TEST(parse_array_should_parse_arrays_with_one_element);
    RUN_TEST(parse_array_should_parse_arrays_with_multiple_elements);
    RUN_TEST(parse_array_should_not_parse_non_arrays);
    return UNITY_END();
}
