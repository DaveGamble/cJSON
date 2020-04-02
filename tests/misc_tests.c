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

static void cjson_get_object_item_should_not_crash_with_array(void) {
    cJSON *array = NULL;
    cJSON *found = NULL;
    array = cJSON_Parse("[1]");

    found = cJSON_GetObjectItem(array, "name");
    TEST_ASSERT_NULL(found);

    cJSON_Delete(array);
}

static void cjson_get_object_item_case_sensitive_should_not_crash_with_array(void) {
    cJSON *array = NULL;
    cJSON *found = NULL;
    array = cJSON_Parse("[1]");

    found = cJSON_GetObjectItemCaseSensitive(array, "name");
    TEST_ASSERT_NULL(found);

    cJSON_Delete(array);
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
    list[0].prev = &(list[3]);

    parent->child = &list[0];

    /* detach in the middle (list[1]) */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_DetachItemViaPointer(parent, &(list[1])) == &(list[1]), "Failed to detach in the middle.");
    TEST_ASSERT_TRUE_MESSAGE((list[1].prev == NULL) && (list[1].next == NULL), "Didn't set pointers of detached item to NULL.");
    TEST_ASSERT_TRUE((list[0].next == &(list[2])) && (list[2].prev == &(list[0])));

    /* detach beginning (list[0]) */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_DetachItemViaPointer(parent, &(list[0])) == &(list[0]), "Failed to detach beginning.");
    TEST_ASSERT_TRUE_MESSAGE((list[0].prev == NULL) && (list[0].next == NULL), "Didn't set pointers of detached item to NULL.");
    TEST_ASSERT_TRUE_MESSAGE((list[2].prev == &(list[3])) && (parent->child == &(list[2])), "Didn't set the new beginning.");

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
    TEST_ASSERT_TRUE(replacements[0].prev == end);
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
    cJSON_bool flag = false;

    child = cJSON_CreateNumber(1);
    TEST_ASSERT_NOT_NULL(child);
    replacement = cJSON_CreateNumber(2);
    TEST_ASSERT_NOT_NULL(replacement);

    flag  = cJSON_AddItemToObject(root, "child", child);
    TEST_ASSERT_TRUE_MESSAGE(flag, "add item to object failed");
    cJSON_ReplaceItemInObject(root, "child", replacement);

    TEST_ASSERT_TRUE(root->child == replacement);
    TEST_ASSERT_EQUAL_STRING("child", replacement->string);

    cJSON_Delete(replacement);
}

static void cjson_functions_should_not_crash_with_null_pointers(void)
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
    TEST_ASSERT_FALSE(cJSON_InsertItemInArray(NULL, 0, item));
    TEST_ASSERT_FALSE(cJSON_InsertItemInArray(item, 0, NULL));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemViaPointer(NULL, item, item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemViaPointer(item, NULL, item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemViaPointer(item, item, NULL));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemInArray(item, 0, NULL));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemInArray(NULL, 0, item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemInObject(NULL, "item", item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemInObject(item, NULL, item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemInObject(item, "item", NULL));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemInObjectCaseSensitive(NULL, "item", item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemInObjectCaseSensitive(item, NULL, item));
    TEST_ASSERT_FALSE(cJSON_ReplaceItemInObjectCaseSensitive(item, "item", NULL));
    TEST_ASSERT_NULL(cJSON_Duplicate(NULL, true));
    TEST_ASSERT_FALSE(cJSON_Compare(item, NULL, false));
    TEST_ASSERT_FALSE(cJSON_Compare(NULL, item, false));
    cJSON_Minify(NULL);
    /* skipped because it is only used via a macro that checks for NULL */
    /* cJSON_SetNumberHelper(NULL, 0); */

    cJSON_Delete(item);
}

static void * CJSON_CDECL failing_realloc(void *pointer, size_t size)
{
    (void)size;
    (void)pointer;
    return NULL;
}

static void ensure_should_fail_on_failed_realloc(void)
{
    printbuffer buffer = {NULL, 10, 0, 0, false, false, {&malloc, &free, &failing_realloc}};
    buffer.buffer = (unsigned char*)malloc(100);
    TEST_ASSERT_NOT_NULL(buffer.buffer);

    TEST_ASSERT_NULL_MESSAGE(ensure(&buffer, 200), "Ensure didn't fail with failing realloc.");
}

static void skip_utf8_bom_should_skip_bom(void)
{
    const unsigned char string[] = "\xEF\xBB\xBF{}";
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = string;
    buffer.length = sizeof(string);
    buffer.hooks = global_hooks;

    TEST_ASSERT_TRUE(skip_utf8_bom(&buffer) == &buffer);
    TEST_ASSERT_EQUAL_UINT(3U, (unsigned int)buffer.offset);
}

static void skip_utf8_bom_should_not_skip_bom_if_not_at_beginning(void)
{
    const unsigned char string[] = " \xEF\xBB\xBF{}";
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    buffer.content = string;
    buffer.length = sizeof(string);
    buffer.hooks = global_hooks;
    buffer.offset = 1;

    TEST_ASSERT_NULL(skip_utf8_bom(&buffer));
}

static void cjson_get_string_value_should_get_a_string(void)
{
    cJSON *string = cJSON_CreateString("test");
    cJSON *number = cJSON_CreateNumber(1);

    TEST_ASSERT_TRUE(cJSON_GetStringValue(string) == string->valuestring);
    TEST_ASSERT_NULL(cJSON_GetStringValue(number));
    TEST_ASSERT_NULL(cJSON_GetStringValue(NULL));

    cJSON_Delete(number);
    cJSON_Delete(string);
}

static void cjson_get_number_value_should_get_a_number(void)
{
    cJSON *string = cJSON_CreateString("test");
    cJSON *number = cJSON_CreateNumber(1);

    TEST_ASSERT_EQUAL_DOUBLE(cJSON_GetNumberValue(number), number->valuedouble);
    TEST_ASSERT_DOUBLE_IS_NAN(cJSON_GetNumberValue(string));
    TEST_ASSERT_DOUBLE_IS_NAN(cJSON_GetNumberValue(NULL));
    
    cJSON_Delete(number);
    cJSON_Delete(string);
}

static void cjson_create_string_reference_should_create_a_string_reference(void) {
    const char *string = "I am a string!";

    cJSON *string_reference = cJSON_CreateStringReference(string);
    TEST_ASSERT_TRUE(string_reference->valuestring == string);
    TEST_ASSERT_EQUAL_INT(cJSON_IsReference | cJSON_String, string_reference->type);

    cJSON_Delete(string_reference);
}

static void cjson_create_object_reference_should_create_an_object_reference(void) {
    cJSON *number_reference = NULL;
    cJSON *number_object = cJSON_CreateObject();
    cJSON *number = cJSON_CreateNumber(42);
    const char key[] = "number";

    TEST_ASSERT_TRUE(cJSON_IsNumber(number));
    TEST_ASSERT_TRUE(cJSON_IsObject(number_object));
    cJSON_AddItemToObjectCS(number_object, key, number);

    number_reference = cJSON_CreateObjectReference(number);
    TEST_ASSERT_TRUE(number_reference->child == number);
    TEST_ASSERT_EQUAL_INT(cJSON_Object | cJSON_IsReference, number_reference->type);

    cJSON_Delete(number_object);
    cJSON_Delete(number_reference);
}

static void cjson_create_array_reference_should_create_an_array_reference(void) {
    cJSON *number_reference = NULL;
    cJSON *number_array = cJSON_CreateArray();
    cJSON *number = cJSON_CreateNumber(42);

    TEST_ASSERT_TRUE(cJSON_IsNumber(number));
    TEST_ASSERT_TRUE(cJSON_IsArray(number_array));
    cJSON_AddItemToArray(number_array, number);

    number_reference = cJSON_CreateArrayReference(number);
    TEST_ASSERT_TRUE(number_reference->child == number);
    TEST_ASSERT_EQUAL_INT(cJSON_Array | cJSON_IsReference, number_reference->type);

    cJSON_Delete(number_array);
    cJSON_Delete(number_reference);
}

static void cjson_add_item_to_object_or_array_should_not_add_itself(void)
{
    cJSON *object = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();
    cJSON_bool flag = false;

    flag = cJSON_AddItemToObject(object, "key", object);
    TEST_ASSERT_FALSE_MESSAGE(flag, "add an object to itself should fail");

    flag = cJSON_AddItemToArray(array, array);
    TEST_ASSERT_FALSE_MESSAGE(flag, "add an array to itself should fail");

    cJSON_Delete(object);
    cJSON_Delete(array);
}

static void cjson_add_item_to_object_should_not_use_after_free_when_string_is_aliased(void)
{
    cJSON *object = cJSON_CreateObject();
    cJSON *number = cJSON_CreateNumber(42);
    char *name = (char*)cJSON_strdup((const unsigned char*)"number", &global_hooks);

    TEST_ASSERT_NOT_NULL(object);
    TEST_ASSERT_NOT_NULL(number);
    TEST_ASSERT_NOT_NULL(name);

    number->string = name;

    /* The following should not have a use after free
     * that would show up in valgrind or with AddressSanitizer */
    cJSON_AddItemToObject(object, number->string, number);

    cJSON_Delete(object);
}

static void cjson_delete_item_from_array_should_not_broken_list_structure(void)
{
    const char expected_json1[] = "{\"rd\":[{\"a\":\"123\"}]}";
    const char expected_json2[] = "{\"rd\":[{\"a\":\"123\"},{\"b\":\"456\"}]}";
    const char expected_json3[] = "{\"rd\":[{\"b\":\"456\"}]}";
    char *str1 = NULL;
    char *str2 = NULL;
    char *str3 = NULL;

    cJSON *root = cJSON_Parse("{}");

    cJSON *array = cJSON_AddArrayToObject(root, "rd");
    cJSON *item1 = cJSON_Parse("{\"a\":\"123\"}");
    cJSON *item2 = cJSON_Parse("{\"b\":\"456\"}");

    cJSON_AddItemToArray(array, item1);
    str1 = cJSON_PrintUnformatted(root);
    TEST_ASSERT_EQUAL_STRING(expected_json1, str1);
    free(str1);

    cJSON_AddItemToArray(array, item2);
    str2 = cJSON_PrintUnformatted(root);
    TEST_ASSERT_EQUAL_STRING(expected_json2, str2);
    free(str2);

    /* this should not broken list structure */
    cJSON_DeleteItemFromArray(array, 0);
    str3 = cJSON_PrintUnformatted(root);
    TEST_ASSERT_EQUAL_STRING(expected_json3, str3);
    free(str3);

    cJSON_Delete(root);
}

static void cjson_set_valuestring_to_object_should_not_leak_memory(void)
{
    cJSON *root = cJSON_Parse("{}");
    const char *stringvalue = "valuestring could be changed safely";
    const char *reference_valuestring = "reference item should be freed by yourself";
    const char *short_valuestring = "shorter valuestring";
    const char *long_valuestring = "new valuestring which much longer than previous should be changed safely";
    cJSON *item1 = cJSON_CreateString(stringvalue);
    cJSON *item2 = cJSON_CreateStringReference(reference_valuestring);
    char *ptr1 = NULL;
    char *return_value = NULL;
    
    cJSON_AddItemToObject(root, "one", item1);
    cJSON_AddItemToObject(root, "two", item2);

    ptr1 = item1->valuestring;
    return_value = cJSON_SetValuestring(cJSON_GetObjectItem(root, "one"), short_valuestring);
    TEST_ASSERT_NOT_NULL(return_value);
    TEST_ASSERT_EQUAL_PTR_MESSAGE(ptr1, return_value, "new valuestring shorter than old should not reallocate memory");
    TEST_ASSERT_EQUAL_STRING(short_valuestring, cJSON_GetObjectItem(root, "one")->valuestring);

    /* we needn't to free the original valuestring manually */
    ptr1 = item1->valuestring;
    return_value = cJSON_SetValuestring(cJSON_GetObjectItem(root, "one"), long_valuestring);
    TEST_ASSERT_NOT_NULL(return_value);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(ptr1, return_value, "new valuestring longer than old should reallocate memory")
    TEST_ASSERT_EQUAL_STRING(long_valuestring, cJSON_GetObjectItem(root, "one")->valuestring);

    return_value = cJSON_SetValuestring(cJSON_GetObjectItem(root, "two"), long_valuestring);
    TEST_ASSERT_NULL_MESSAGE(return_value, "valuestring of reference object should not be changed");
    TEST_ASSERT_EQUAL_STRING(reference_valuestring, cJSON_GetObjectItem(root, "two")->valuestring);

    cJSON_Delete(root);
}

int CJSON_CDECL main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_array_foreach_should_loop_over_arrays);
    RUN_TEST(cjson_array_foreach_should_not_dereference_null_pointer);
    RUN_TEST(cjson_get_object_item_should_get_object_items);
    RUN_TEST(cjson_get_object_item_case_sensitive_should_get_object_items);
    RUN_TEST(cjson_get_object_item_should_not_crash_with_array);
    RUN_TEST(cjson_get_object_item_case_sensitive_should_not_crash_with_array);
    RUN_TEST(typecheck_functions_should_check_type);
    RUN_TEST(cjson_should_not_parse_to_deeply_nested_jsons);
    RUN_TEST(cjson_set_number_value_should_set_numbers);
    RUN_TEST(cjson_detach_item_via_pointer_should_detach_items);
    RUN_TEST(cjson_replace_item_via_pointer_should_replace_items);
    RUN_TEST(cjson_replace_item_in_object_should_preserve_name);
    RUN_TEST(cjson_functions_should_not_crash_with_null_pointers);
    RUN_TEST(ensure_should_fail_on_failed_realloc);
    RUN_TEST(skip_utf8_bom_should_skip_bom);
    RUN_TEST(skip_utf8_bom_should_not_skip_bom_if_not_at_beginning);
    RUN_TEST(cjson_get_string_value_should_get_a_string);
    RUN_TEST(cjson_get_number_value_should_get_a_number);
    RUN_TEST(cjson_create_string_reference_should_create_a_string_reference);
    RUN_TEST(cjson_create_object_reference_should_create_an_object_reference);
    RUN_TEST(cjson_create_array_reference_should_create_an_array_reference);
    RUN_TEST(cjson_add_item_to_object_or_array_should_not_add_itself);
    RUN_TEST(cjson_add_item_to_object_should_not_use_after_free_when_string_is_aliased);
    RUN_TEST(cjson_delete_item_from_array_should_not_broken_list_structure);
    RUN_TEST(cjson_set_valuestring_to_object_should_not_leak_memory);

    return UNITY_END();
}
