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

static void assert_is_int64(cJSON *int64_number_item)
{
    assert_has_type(int64_number_item, cJSON_Number);
    TEST_ASSERT_BITS_MESSAGE(cJSON_IsInt64, cJSON_IsInt64, int64_number_item->type, "Item should be a int64 integer.");
}

static void cjson_get_object_item_should_get_object_items_with_int64(void)
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
    assert_is_int64(found);

    found = cJSON_GetObjectItem(item, "tWo");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find first item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 2);
    assert_is_int64(found);

    found = cJSON_GetObjectItem(item, "three");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 3);
    assert_is_int64(found);

    found = cJSON_GetObjectItem(item, "four");
    TEST_ASSERT_NULL_MESSAGE(found, "Should not find something that isn't there.");

    cJSON_Delete(item);
}

static void cjson_get_object_item_case_sensitive_should_get_object_items_with_int64(void)
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
    assert_is_int64(found);

    found = cJSON_GetObjectItemCaseSensitive(item, "Two");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find first item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 2);
    assert_is_int64(found);

    found = cJSON_GetObjectItemCaseSensitive(item, "tHree");
    TEST_ASSERT_NOT_NULL_MESSAGE(found, "Failed to find item.");
    TEST_ASSERT_EQUAL_DOUBLE(found->valuedouble, 3);
    assert_is_int64(found);

    found = cJSON_GetObjectItemCaseSensitive(item, "One");
    TEST_ASSERT_NULL_MESSAGE(found, "Should not find something that isn't there.");

    cJSON_Delete(item);
}

static void cjson_set_number_value_should_set_numbers_with_int64(void)
{
    cJSON number[1] = {{NULL, NULL, NULL, cJSON_Number | cJSON_IsInt64, NULL, 0, 0, NULL}};

    cJSON_SetInt64NumberValue(number, 1LL);
    TEST_ASSERT_EQUAL_INT64(1LL, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, number->valuedouble);

    cJSON_SetInt64NumberValue(number, -1LL);
    TEST_ASSERT_EQUAL_INT64(-1LL, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(-1.0, number->valuedouble);

    cJSON_SetInt64NumberValue(number, 0LL);
    TEST_ASSERT_EQUAL_INT64(0LL, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, number->valuedouble);

    cJSON_SetInt64NumberValue(number, LLONG_MAX);
    TEST_ASSERT_EQUAL_INT64(LLONG_MAX, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE((double)LLONG_MAX, number->valuedouble);

    cJSON_SetInt64NumberValue(number, LLONG_MIN);
    TEST_ASSERT_EQUAL_INT64(LLONG_MIN, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE((double)LLONG_MIN, number->valuedouble);
}

static void typecheck_functions_should_check_type_with_int64(void)
{
    cJSON item[1];
    item->type = cJSON_Number;
    TEST_ASSERT_FALSE(cJSON_IsInt64Number(item));

    item->type = cJSON_IsInt64;
    TEST_ASSERT_FALSE(cJSON_IsInt64Number(item));

    item->type = cJSON_Number | cJSON_IsInt64;
    TEST_ASSERT_TRUE(cJSON_IsInt64Number(item));

    item->type = cJSON_False;
    TEST_ASSERT_FALSE(cJSON_IsInt64Number(item));
}

static void cjson_functions_should_not_crash_with_null_pointers_with_int64(void)
{
    cJSON *item = cJSON_CreateString("item");

    TEST_ASSERT_FALSE(cJSON_IsInt64Number(NULL));
    cJSON_AddInt64NumberToObject(NULL, "item", 0LL);
    cJSON_AddInt64NumberToObject(item, NULL, 0LL);
    cJSON_AddInt64NumberToObject(NULL, NULL, 0LL);
    TEST_ASSERT_NULL(cJSON_GetInt64NumberValue(NULL));
    TEST_ASSERT_EQUAL_INT64(0LL, cJSON_SetInt64NumberValue(NULL, 0LL));

    cJSON_Delete(item);
}

static void cjson_get_number_value_should_get_a_number_with_int64(void)
{
    cJSON *string = cJSON_CreateString("test");
    cJSON *number = cJSON_CreateInt64Number(1LL);

    TEST_ASSERT_EQUAL_INT64(*cJSON_GetInt64NumberValue(number), number->valueint);
    TEST_ASSERT_NULL(cJSON_GetInt64NumberValue(string));
    TEST_ASSERT_NULL(cJSON_GetInt64NumberValue(NULL));

    cJSON_Delete(number);
    cJSON_Delete(string);
}

int CJSON_CDECL main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_get_object_item_should_get_object_items_with_int64);
    RUN_TEST(cjson_get_object_item_case_sensitive_should_get_object_items_with_int64);
    RUN_TEST(cjson_set_number_value_should_set_numbers_with_int64);
    RUN_TEST(typecheck_functions_should_check_type_with_int64);
    RUN_TEST(cjson_functions_should_not_crash_with_null_pointers_with_int64);
    RUN_TEST(cjson_get_number_value_should_get_a_number_with_int64);

    return UNITY_END();
}
