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

static void cjson_should_not_parse_to_deeply_nested_jsons(void)
{
    char deep_json[CJSON_NESTING_LIMIT + 1];
    size_t position = 0;

    for (position = 0; position < sizeof(deep_json); position++)
    {
        deep_json[position] = '[';
    }
    deep_json[sizeof(deep_json) - 1] = '\0';

    TEST_ASSERT_NULL_MESSAGE(cJSON_Parse(deep_json), "To deep JSONs should not be parsed.");
}

static void cjson_set_number_value_should_set_numbers(void)
{
    cJSON number[1] = {{NULL, NULL, NULL, cJSON_Number, NULL, 0, 0, NULL}};

    cJSON_SetNumberValue(number, 1.5);
    TEST_ASSERT_EQUAL(1, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(1.5, number->valuedouble);

    cJSON_SetNumberValue(number, -1.5);
    TEST_ASSERT_EQUAL(-1, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(-1.5, number->valuedouble);

    cJSON_SetNumberValue(number, 1 + (double)INT_MAX);
    TEST_ASSERT_EQUAL(INT_MAX, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(1 + (double)INT_MAX, number->valuedouble);

    cJSON_SetNumberValue(number, -1 + (double)INT_MIN);
    TEST_ASSERT_EQUAL(INT_MIN, number->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(-1 + (double)INT_MIN, number->valuedouble);
}

static void cjson_detach_item_via_pointer_should_detach_items(void)
{
    cJSON list[4];
    cJSON parent[1];

    memset(list, '\0', sizeof(list));

    /* link the list */
    list[0].next = &(list[1]);
    list[1].next = &(list[2]);
    list[2].next = &(list[3]);

    list[3].prev = &(list[2]);
    list[2].prev = &(list[1]);
    list[1].prev = &(list[0]);

    parent->child = &list[0];

    /* detach in the middle (list[1]) */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_DetachItemViaPointer(parent, &(list[1])) == &(list[1]), "Failed to detach in the middle.");
    TEST_ASSERT_TRUE_MESSAGE((list[1].prev == NULL) && (list[1].next == NULL), "Didn't set pointers of detached item to NULL.");
    TEST_ASSERT_TRUE((list[0].next == &(list[2])) && (list[2].prev == &(list[0])));

    /* detach beginning (list[0]) */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_DetachItemViaPointer(parent, &(list[0])) == &(list[0]), "Failed to detach beginning.");
    TEST_ASSERT_TRUE_MESSAGE((list[0].prev == NULL) && (list[0].next == NULL), "Didn't set pointers of detached item to NULL.");
    TEST_ASSERT_TRUE_MESSAGE((list[2].prev == NULL) && (parent->child == &(list[2])), "Didn't set the new beginning.");

    /* detach end (list[3])*/
    TEST_ASSERT_TRUE_MESSAGE(cJSON_DetachItemViaPointer(parent, &(list[3])) == &(list[3]), "Failed to detach end.");
    TEST_ASSERT_TRUE_MESSAGE((list[3].prev == NULL) && (list[3].next == NULL), "Didn't set pointers of detached item to NULL.");
    TEST_ASSERT_TRUE_MESSAGE((list[2].next == NULL) && (parent->child == &(list[2])), "Didn't set the new end");

    /* detach single item (list[2]) */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_DetachItemViaPointer(parent, &list[2]) == &list[2], "Failed to detach single item.");
    TEST_ASSERT_TRUE_MESSAGE((list[2].prev == NULL) && (list[2].next == NULL), "Didn't set pointers of detached item to NULL.");
    TEST_ASSERT_NULL_MESSAGE(parent->child, "Child of the parent wasn't set to NULL.");
}

static void cjson_replace_item_via_pointer_should_replace_items(void)
{
    cJSON replacements[3];
    cJSON *beginning = NULL;
    cJSON *middle = NULL;
    cJSON *end = NULL;
    cJSON *array = NULL;

    beginning = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(beginning);
    middle = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(middle);
    end = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(end);

    array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array);

    cJSON_AddItemToArray(array, beginning);
    cJSON_AddItemToArray(array, middle);
    cJSON_AddItemToArray(array, end);


    memset(replacements, '\0', sizeof(replacements));

    /* replace beginning */
    TEST_ASSERT_TRUE(cJSON_ReplaceItemViaPointer(array, beginning, &(replacements[0])));
    TEST_ASSERT_NULL(replacements[0].prev);
    TEST_ASSERT_TRUE(replacements[0].next == middle);
    TEST_ASSERT_TRUE(middle->prev == &(replacements[0]));
    TEST_ASSERT_TRUE(array->child == &(replacements[0]));

    /* replace middle */
    TEST_ASSERT_TRUE(cJSON_ReplaceItemViaPointer(array, middle, &(replacements[1])));
    TEST_ASSERT_TRUE(replacements[1].prev == &(replacements[0]));
    TEST_ASSERT_TRUE(replacements[1].next == end);
    TEST_ASSERT_TRUE(end->prev == &(replacements[1]));

    /* replace end */
    TEST_ASSERT_TRUE(cJSON_ReplaceItemViaPointer(array, end, &(replacements[2])));
    TEST_ASSERT_TRUE(replacements[2].prev == &(replacements[1]));
    TEST_ASSERT_NULL(replacements[2].next);
    TEST_ASSERT_TRUE(replacements[1].next == &(replacements[2]));

    cJSON_free(array);
}

static void cjson_replace_item_in_object_should_preserve_name(void)
{
    cJSON root[1] = {{ NULL, NULL, NULL, 0, NULL, 0, 0, NULL }};
    cJSON *child = NULL;
    cJSON *replacement = NULL;

    child = cJSON_CreateNumber(1);
    TEST_ASSERT_NOT_NULL(child);
    replacement = cJSON_CreateNumber(2);
    TEST_ASSERT_NOT_NULL(replacement);

    cJSON_AddItemToObject(root, "child", child);
    cJSON_ReplaceItemInObject(root, "child", replacement);

    TEST_ASSERT_TRUE(root->child == replacement);
    TEST_ASSERT_EQUAL_STRING("child", replacement->string);

    cJSON_Delete(replacement);
}

static void cjson_functions_shouldnt_crash_with_null_pointers(void)
{
    char buffer[10];
    cJSON *item = cJSON_CreateString("item");

    cJSON_InitHooks(NULL);
    TEST_ASSERT_NULL(cJSON_Parse(NULL));
    TEST_ASSERT_NULL(cJSON_ParseWithOpts(NULL, NULL, true));
    TEST_ASSERT_NULL(cJSON_Print(NULL));
    TEST_ASSERT_NULL(cJSON_PrintUnformatted(NULL));
    TEST_ASSERT_NULL(cJSON_PrintBuffered(NULL, 10, true));
    TEST_ASSERT_FALSE(cJSON_PrintPreallocated(NULL, buffer, sizeof(buffer), true));
    TEST_ASSERT_FALSE(cJSON_PrintPreallocated(item, NULL, 1, true));
    cJSON_Delete(NULL);
    cJSON_GetArraySize(NULL);
    TEST_ASSERT_NULL(cJSON_GetArrayItem(NULL, 0));
    TEST_ASSERT_NULL(cJSON_GetObjectItem(NULL, "item"));
    TEST_ASSERT_NULL(cJSON_GetObjectItem(item, NULL));
    TEST_ASSERT_NULL(cJSON_GetObjectItemCaseSensitive(NULL, "item"));
    TEST_ASSERT_NULL(cJSON_GetObjectItemCaseSensitive(item, NULL));
    TEST_ASSERT_FALSE(cJSON_HasObjectItem(NULL, "item"));
    TEST_ASSERT_FALSE(cJSON_HasObjectItem(item, NULL));
    TEST_ASSERT_FALSE(cJSON_IsInvalid(NULL));
    TEST_ASSERT_FALSE(cJSON_IsFalse(NULL));
    TEST_ASSERT_FALSE(cJSON_IsTrue(NULL));
    TEST_ASSERT_FALSE(cJSON_IsBool(NULL));
    TEST_ASSERT_FALSE(cJSON_IsNull(NULL));
    TEST_ASSERT_FALSE(cJSON_IsNumber(NULL));
    TEST_ASSERT_FALSE(cJSON_IsString(NULL));
    TEST_ASSERT_FALSE(cJSON_IsArray(NULL));
    TEST_ASSERT_FALSE(cJSON_IsObject(NULL));
    TEST_ASSERT_FALSE(cJSON_IsRaw(NULL));
    TEST_ASSERT_NULL(cJSON_CreateString(NULL));
    TEST_ASSERT_NULL(cJSON_CreateRaw(NULL));
    TEST_ASSERT_NULL(cJSON_CreateIntArray(NULL, 10));
    TEST_ASSERT_NULL(cJSON_CreateFloatArray(NULL, 10));
    TEST_ASSERT_NULL(cJSON_CreateDoubleArray(NULL, 10));
    TEST_ASSERT_NULL(cJSON_CreateStringArray(NULL, 10));
    cJSON_AddItemToArray(NULL, item);
    cJSON_AddItemToArray(item, NULL);
    cJSON_AddItemToObject(item, "item", NULL);
    cJSON_AddItemToObject(item, NULL, item);
    cJSON_AddItemToObject(NULL, "item", item);
    cJSON_AddItemToObjectCS(item, "item", NULL);
    cJSON_AddItemToObjectCS(item, NULL, item);
    cJSON_AddItemToObjectCS(NULL, "item", item);
    cJSON_AddItemReferenceToArray(NULL, item);
    cJSON_AddItemReferenceToArray(item, NULL);
    cJSON_AddItemReferenceToObject(item, "item", NULL);
    cJSON_AddItemReferenceToObject(item, NULL, item);
    cJSON_AddItemReferenceToObject(NULL, "item", item);
    TEST_ASSERT_NULL(cJSON_DetachItemViaPointer(NULL, item));
    TEST_ASSERT_NULL(cJSON_DetachItemViaPointer(item, NULL));
    TEST_ASSERT_NULL(cJSON_DetachItemFromArray(NULL, 0));
    cJSON_DeleteItemFromArray(NULL, 0);
    TEST_ASSERT_NULL(cJSON_DetachItemFromObject(NULL, "item"));
    TEST_ASSERT_NULL(cJSON_DetachItemFromObject(item, NULL));
    TEST_ASSERT_NULL(cJSON_DetachItemFromObjectCaseSensitive(NULL, "item"));
    TEST_ASSERT_NULL(cJSON_DetachItemFromObjectCaseSensitive(item, NULL));
    cJSON_DeleteItemFromObject(NULL, "item");
    cJSON_DeleteItemFromObject(item, NULL);
    cJSON_DeleteItemFromObjectCaseSensitive(NULL, "item");
    cJSON_DeleteItemFromObjectCaseSensitive(item, NULL);
    cJSON_InsertItemInArray(NULL, 0, item);
    cJSON_InsertItemInArray(item, 0, NULL);
    TEST_ASSERT_FALSE(cJSON_ReplaceItemViaPointer(NULL, item, item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemViaPointer(item, NULL, item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemViaPointer(item, item, NULL));
    cJSON_ReplaceItemInArray(item, 0, NULL);
    cJSON_ReplaceItemInArray(NULL, 0, item);
    cJSON_ReplaceItemInObject(NULL, "item", item);
    cJSON_ReplaceItemInObject(item, NULL, item);
    cJSON_ReplaceItemInObject(item, "item", NULL);
    cJSON_ReplaceItemInObjectCaseSensitive(NULL, "item", item);
    cJSON_ReplaceItemInObjectCaseSensitive(item, NULL, item);
    cJSON_ReplaceItemInObjectCaseSensitive(item, "item", NULL);
    TEST_ASSERT_NULL(cJSON_Duplicate(NULL, true));
    TEST_ASSERT_FALSE(cJSON_Compare(item, NULL, false));
    TEST_ASSERT_FALSE(cJSON_Compare(NULL, item, false));
    cJSON_Minify(NULL);
    /* skipped because it is only used via a macro that checks for NULL */
    /* cJSON_SetNumberHelper(NULL, 0); */

    cJSON_Delete(item);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_array_foreach_should_loop_over_arrays);
    RUN_TEST(cjson_array_foreach_should_not_dereference_null_pointer);
    RUN_TEST(cjson_get_object_item_should_get_object_items);
    RUN_TEST(cjson_get_object_item_case_sensitive_should_get_object_items);
    RUN_TEST(typecheck_functions_should_check_type);
    RUN_TEST(cjson_should_not_parse_to_deeply_nested_jsons);
    RUN_TEST(cjson_set_number_value_should_set_numbers);
    RUN_TEST(cjson_detach_item_via_pointer_should_detach_items);
    RUN_TEST(cjson_replace_item_via_pointer_should_replace_items);
    RUN_TEST(cjson_replace_item_in_object_should_preserve_name);
    RUN_TEST(cjson_functions_shouldnt_crash_with_null_pointers);

    return UNITY_END();
}
