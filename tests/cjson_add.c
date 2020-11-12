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

static void * CJSON_CDECL failing_malloc(size_t size)
{
    (void)size;
    return NULL;
}

/* work around MSVC error C2322: '...' address of dillimport '...' is not static */
static void CJSON_CDECL normal_free(void *pointer)
{
    free(pointer);
}

static cJSON_Hooks failing_hooks = {
    failing_malloc,
    normal_free
};

static void cjson_add_null_should_add_null(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *null = NULL;

    cJSON_AddNullToObject(root, "null");

    TEST_ASSERT_NOT_NULL(null = cJSON_GetObjectItemCaseSensitive(root, "null"));
    TEST_ASSERT_EQUAL_INT(null->type, cJSON_NULL);

    cJSON_Delete(root);
}

static void cjson_add_null_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddNullToObject(NULL, "null"));
    TEST_ASSERT_NULL(cJSON_AddNullToObject(root, NULL));

    cJSON_Delete(root);
}

static void cjson_add_null_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddNullToObject(root, "null"));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

static void cjson_add_true_should_add_true(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *true_item = NULL;

    cJSON_AddTrueToObject(root, "true");

    TEST_ASSERT_NOT_NULL(true_item = cJSON_GetObjectItemCaseSensitive(root, "true"));
    TEST_ASSERT_EQUAL_INT(true_item->type, cJSON_True);

    cJSON_Delete(root);
}

static void cjson_add_true_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddTrueToObject(NULL, "true"));
    TEST_ASSERT_NULL(cJSON_AddTrueToObject(root, NULL));

    cJSON_Delete(root);
}

static void cjson_add_true_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddTrueToObject(root, "true"));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

static void cjson_create_int_array_should_fail_on_allocation_failure(void)
{
    int numbers[] = {1, 2, 3};

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_CreateIntArray(numbers, 3));

    cJSON_InitHooks(NULL);
}

static void cjson_create_float_array_should_fail_on_allocation_failure(void)
{
    float numbers[] = {1.0f, 2.0f, 3.0f};

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_CreateFloatArray(numbers, 3));

    cJSON_InitHooks(NULL);
}

static void cjson_create_double_array_should_fail_on_allocation_failure(void)
{
    double numbers[] = {1.0, 2.0, 3.0};

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_CreateDoubleArray(numbers, 3));

    cJSON_InitHooks(NULL);
}

static void cjson_create_string_array_should_fail_on_allocation_failure(void)
{
    const char* strings[] = {"1", "2", "3"};

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_CreateStringArray(strings, 3));

    cJSON_InitHooks(NULL);
}

static void cjson_add_false_should_add_false(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *false_item = NULL;

    cJSON_AddFalseToObject(root, "false");

    TEST_ASSERT_NOT_NULL(false_item = cJSON_GetObjectItemCaseSensitive(root, "false"));
    TEST_ASSERT_EQUAL_INT(false_item->type, cJSON_False);

    cJSON_Delete(root);
}

static void cjson_add_false_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddFalseToObject(NULL, "false"));
    TEST_ASSERT_NULL(cJSON_AddFalseToObject(root, NULL));

    cJSON_Delete(root);
}

static void cjson_add_false_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddFalseToObject(root, "false"));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

static void cjson_add_bool_should_add_bool(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *true_item = NULL;
    cJSON *false_item = NULL;

    /* true */
    cJSON_AddBoolToObject(root, "true", true);
    TEST_ASSERT_NOT_NULL(true_item = cJSON_GetObjectItemCaseSensitive(root, "true"));
    TEST_ASSERT_EQUAL_INT(true_item->type, cJSON_True);

    /* false */
    cJSON_AddBoolToObject(root, "false", false);
    TEST_ASSERT_NOT_NULL(false_item = cJSON_GetObjectItemCaseSensitive(root, "false"));
    TEST_ASSERT_EQUAL_INT(false_item->type, cJSON_False);

    cJSON_Delete(root);
}

static void cjson_add_bool_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddBoolToObject(NULL, "false", false));
    TEST_ASSERT_NULL(cJSON_AddBoolToObject(root, NULL, false));

    cJSON_Delete(root);
}

static void cjson_add_bool_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddBoolToObject(root, "false", false));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

static void cjson_add_number_should_add_number(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *number = NULL;

    cJSON_AddNumberToObject(root, "number", 42);

    TEST_ASSERT_NOT_NULL(number = cJSON_GetObjectItemCaseSensitive(root, "number"));

    TEST_ASSERT_EQUAL_INT(number->type, cJSON_Number);
    TEST_ASSERT_EQUAL_DOUBLE(number->valuedouble, 42);
    TEST_ASSERT_EQUAL_INT(number->valueint, 42);

    cJSON_Delete(root);
}

static void cjson_add_number_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddNumberToObject(NULL, "number", 42));
    TEST_ASSERT_NULL(cJSON_AddNumberToObject(root, NULL, 42));

    cJSON_Delete(root);
}

static void cjson_add_number_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddNumberToObject(root, "number", 42));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

static void cjson_add_string_should_add_string(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *string = NULL;

    cJSON_AddStringToObject(root, "string", "Hello World!");

    TEST_ASSERT_NOT_NULL(string = cJSON_GetObjectItemCaseSensitive(root, "string"));
    TEST_ASSERT_EQUAL_INT(string->type, cJSON_String);
    TEST_ASSERT_EQUAL_STRING(string->valuestring, "Hello World!");

    cJSON_Delete(root);
}

static void cjson_add_string_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddStringToObject(NULL, "string", "string"));
    TEST_ASSERT_NULL(cJSON_AddStringToObject(root, NULL, "string"));

    cJSON_Delete(root);
}

static void cjson_add_string_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddStringToObject(root, "string", "string"));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

static void cjson_add_raw_should_add_raw(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *raw = NULL;

    cJSON_AddRawToObject(root, "raw", "{}");

    TEST_ASSERT_NOT_NULL(raw = cJSON_GetObjectItemCaseSensitive(root, "raw"));
    TEST_ASSERT_EQUAL_INT(raw->type, cJSON_Raw);
    TEST_ASSERT_EQUAL_STRING(raw->valuestring, "{}");

    cJSON_Delete(root);
}

static void cjson_add_raw_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddRawToObject(NULL, "raw", "{}"));
    TEST_ASSERT_NULL(cJSON_AddRawToObject(root, NULL, "{}"));

    cJSON_Delete(root);
}

static void cjson_add_raw_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddRawToObject(root, "raw", "{}"));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

static void cJSON_add_object_should_add_object(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *object = NULL;

    cJSON_AddObjectToObject(root, "object");
    TEST_ASSERT_NOT_NULL(object = cJSON_GetObjectItemCaseSensitive(root, "object"));
    TEST_ASSERT_EQUAL_INT(object->type, cJSON_Object);

    cJSON_Delete(root);
}

static void cjson_add_object_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddObjectToObject(NULL, "object"));
    TEST_ASSERT_NULL(cJSON_AddObjectToObject(root, NULL));

    cJSON_Delete(root);
}

static void cjson_add_object_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddObjectToObject(root, "object"));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

static void cJSON_add_array_should_add_array(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *array = NULL;

    cJSON_AddArrayToObject(root, "array");
    TEST_ASSERT_NOT_NULL(array = cJSON_GetObjectItemCaseSensitive(root, "array"));
    TEST_ASSERT_EQUAL_INT(array->type, cJSON_Array);

    cJSON_Delete(root);
}

static void cjson_add_array_should_fail_with_null_pointers(void)
{
    cJSON *root = cJSON_CreateObject();

    TEST_ASSERT_NULL(cJSON_AddArrayToObject(NULL, "array"));
    TEST_ASSERT_NULL(cJSON_AddArrayToObject(root, NULL));

    cJSON_Delete(root);
}

static void cjson_add_array_should_fail_on_allocation_failure(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_InitHooks(&failing_hooks);

    TEST_ASSERT_NULL(cJSON_AddArrayToObject(root, "array"));

    cJSON_InitHooks(NULL);

    cJSON_Delete(root);
}

int CJSON_CDECL main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_add_null_should_add_null);
    RUN_TEST(cjson_add_null_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_null_should_fail_on_allocation_failure);

    RUN_TEST(cjson_add_true_should_add_true);
    RUN_TEST(cjson_add_true_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_true_should_fail_on_allocation_failure);

    RUN_TEST(cjson_create_int_array_should_fail_on_allocation_failure);
    RUN_TEST(cjson_create_float_array_should_fail_on_allocation_failure);
    RUN_TEST(cjson_create_double_array_should_fail_on_allocation_failure);
    RUN_TEST(cjson_create_string_array_should_fail_on_allocation_failure);

    RUN_TEST(cjson_add_false_should_add_false);
    RUN_TEST(cjson_add_false_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_false_should_fail_on_allocation_failure);

    RUN_TEST(cjson_add_bool_should_add_bool);
    RUN_TEST(cjson_add_bool_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_bool_should_fail_on_allocation_failure);

    RUN_TEST(cjson_add_number_should_add_number);
    RUN_TEST(cjson_add_number_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_number_should_fail_on_allocation_failure);

    RUN_TEST(cjson_add_string_should_add_string);
    RUN_TEST(cjson_add_string_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_string_should_fail_on_allocation_failure);

    RUN_TEST(cjson_add_raw_should_add_raw);
    RUN_TEST(cjson_add_raw_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_raw_should_fail_on_allocation_failure);

    RUN_TEST(cJSON_add_object_should_add_object);
    RUN_TEST(cjson_add_object_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_object_should_fail_on_allocation_failure);

    RUN_TEST(cJSON_add_array_should_add_array);
    RUN_TEST(cjson_add_array_should_fail_with_null_pointers);
    RUN_TEST(cjson_add_array_should_fail_on_allocation_failure);

    return UNITY_END();
}
