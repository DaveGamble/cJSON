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


static void cjson_array_foreach_should_loop_over_arrays(void)
{
    cJSON array[1];
    cJSON elements[10];
    cJSON *element_pointer = NULL;
    size_t i = 0;

    memset(array, 0, sizeof(array));
    memset(elements, 0, sizeof(elements));

    /* create array */
    array[0].child = &elements[0];
    elements[0].prev = NULL;
    elements[9].next = NULL;
    for (i = 0; i < 9; i++)
    {
        elements[i].next = &elements[i + 1];
        elements[i + 1].prev = &elements[i];
    }

    i = 0;
    cJSON_ArrayForEach(element_pointer, array)
    {
        TEST_ASSERT_TRUE_MESSAGE(element_pointer == &elements[i], "Not iterating over array properly");
        i++;
    }
}

static void cjson_array_foreach_should_not_dereference_null_pointer(void)
{
    cJSON *array = NULL;
    cJSON *element = NULL;
    cJSON_ArrayForEach(element, array);
}

static void cjson_get_object_item_should_get_object_items(void)
{
    cJSON *item = NULL;
    cJSON *found = NULL;

    item = cJSON_Parse("{\"one\":1, \"Two\":2, \"tHree\":3}");

    found = cJSON_GetObjectItem(NULL, "test");
    TEST_ASSERT_NULL_MESSAGE(found, "Failed to fail on NULL pointer.");

    found = cJSON_GetObjectItem(item, NULL);
    TEST_ASSERT_NULL_MESSAGE(found, "Failed to fail on NULL string.");


    found = cJSON_GetObjectItem(item, "one");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find first item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 1);

    found = cJSON_GetObjectItem(item, "tWo");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find first item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 2);

    found = cJSON_GetObjectItem(item, "three");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 3);

    found = cJSON_GetObjectItem(item, "four");
    TEST_ASSERT_NULL_MESSAGE(found, "Should not find something that isn't there.");

    cJSON_Delete(item);
}

static void cjson_get_object_item_case_sensitive_should_get_object_items(void)
{
    cJSON *item = NULL;
    cJSON *found = NULL;

    item = cJSON_Parse("{\"one\":1, \"Two\":2, \"tHree\":3}");

    found = cJSON_GetObjectItemCaseSensitive(NULL, "test");
    TEST_ASSERT_NULL_MESSAGE(found, "Failed to fail on NULL pointer.");

    found = cJSON_GetObjectItemCaseSensitive(item, NULL);
    TEST_ASSERT_NULL_MESSAGE(found, "Failed to fail on NULL string.");

    found = cJSON_GetObjectItemCaseSensitive(item, "one");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find first item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 1);

    found = cJSON_GetObjectItemCaseSensitive(item, "Two");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find first item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 2);

    found = cJSON_GetObjectItemCaseSensitive(item, "tHree");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 3);

    found = cJSON_GetObjectItemCaseSensitive(item, "One");
    TEST_ASSERT_NULL_MESSAGE(found, "Should not find something that isn't there.");

    cJSON_Delete(item);
}

static void typecheck_functions_should_check_type(void)
{
    cJSON invalid[1];
    cJSON item[1];
    invalid->type = cJSON_Invalid;
    invalid->type |= cJSON_StringIsConst;
    item->type = cJSON_False;
    item->type |= cJSON_StringIsConst;

    TEST_ASSERT_FALSE(cJSON_IsInvalid(NULL));
    TEST_ASSERT_FALSE(cJSON_IsInvalid(item));
    TEST_ASSERT_TRUE(cJSON_IsInvalid(invalid));

    item->type = cJSON_False | cJSON_StringIsConst;
    TEST_ASSERT_FALSE(cJSON_IsFalse(NULL));
    TEST_ASSERT_FALSE(cJSON_IsFalse(invalid));
    TEST_ASSERT_TRUE(cJSON_IsFalse(item));
    TEST_ASSERT_TRUE(cJSON_IsBool(item));

    item->type = cJSON_True | cJSON_StringIsConst;
    TEST_ASSERT_FALSE(cJSON_IsTrue(NULL));
    TEST_ASSERT_FALSE(cJSON_IsTrue(invalid));
    TEST_ASSERT_TRUE(cJSON_IsTrue(item));
    TEST_ASSERT_TRUE(cJSON_IsBool(item));

    item->type = cJSON_NULL | cJSON_StringIsConst;
    TEST_ASSERT_FALSE(cJSON_IsNull(NULL));
    TEST_ASSERT_FALSE(cJSON_IsNull(invalid));
    TEST_ASSERT_TRUE(cJSON_IsNull(item));

    item->type = cJSON_Number | cJSON_StringIsConst;
    TEST_ASSERT_FALSE(cJSON_IsNumber(NULL));
    TEST_ASSERT_FALSE(cJSON_IsNumber(invalid));
    TEST_ASSERT_TRUE(cJSON_IsNumber(item));

    item->type = cJSON_String | cJSON_StringIsConst;
    TEST_ASSERT_FALSE(cJSON_IsString(NULL));
    TEST_ASSERT_FALSE(cJSON_IsString(invalid));
    TEST_ASSERT_TRUE(cJSON_IsString(item));

    item->type = cJSON_Array | cJSON_StringIsConst;
    TEST_ASSERT_FALSE(cJSON_IsArray(NULL));
    TEST_ASSERT_FALSE(cJSON_IsArray(invalid));
    TEST_ASSERT_TRUE(cJSON_IsArray(item));

    item->type = cJSON_Object | cJSON_StringIsConst;
    TEST_ASSERT_FALSE(cJSON_IsObject(NULL));
    TEST_ASSERT_FALSE(cJSON_IsObject(invalid));
    TEST_ASSERT_TRUE(cJSON_IsObject(item));

    item->type = cJSON_Raw | cJSON_StringIsConst;
    TEST_ASSERT_FALSE(cJSON_IsRaw(NULL));
    TEST_ASSERT_FALSE(cJSON_IsRaw(invalid));
    TEST_ASSERT_TRUE(cJSON_IsRaw(item));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_array_foreach_should_loop_over_arrays);
    RUN_TEST(cjson_array_foreach_should_not_dereference_null_pointer);
    RUN_TEST(cjson_get_object_item_should_get_object_items);
    RUN_TEST(cjson_get_object_item_case_sensitive_should_get_object_items);
    RUN_TEST(typecheck_functions_should_check_type);

    return UNITY_END();
}
