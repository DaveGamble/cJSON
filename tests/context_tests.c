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

static void create_context_should_create_a_context(void)
{
    internal_context *context = NULL;

    context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);
    TEST_ASSERT_EQUAL_MESSAGE(context->buffer_size, 256, "buffer_size has an incorrect value.");
    TEST_ASSERT_TRUE_MESSAGE(context->format, "format has an incorrect value.");
    TEST_ASSERT_TRUE_MESSAGE(context->case_sensitive, "case_sensitive has an incorrect value.");
    TEST_ASSERT_TRUE_MESSAGE(context->allow_data_after_json, "allow_data_after_json has an incorrect value.");
    TEST_ASSERT_NULL_MESSAGE(context->userdata, "Userdata should be NULL");
    TEST_ASSERT_TRUE_MESSAGE(context->duplicate_recursive, "Duplicating is not recursive.");
    TEST_ASSERT_TRUE_MESSAGE(malloc_wrapper == context->allocators.allocate, "Wrong malloc.");
    TEST_ASSERT_TRUE_MESSAGE(realloc_wrapper == context->allocators.reallocate, "Wrong realloc.");
    TEST_ASSERT_TRUE_MESSAGE(free_wrapper == context->allocators.deallocate, "Wrong free.");

    free(context);
}

static void* custom_allocator(size_t size, void *userdata)
{
    *((size_t*)userdata) = size;
    return malloc(size);
}
static void custom_deallocator(void *pointer, void *userdata)
{
    *((size_t*)userdata) = (size_t)pointer;
    free(pointer);
}

static void create_context_should_take_custom_allocators(void)
{
    internal_context *context = NULL;
    cJSON_Allocators allocators = {custom_allocator, custom_deallocator, NULL};
    size_t userdata = 0;

    context = (internal_context*)cJSON_CreateContext(&allocators, &userdata);
    TEST_ASSERT_NOT_NULL(context);
    TEST_ASSERT_EQUAL_MESSAGE(userdata, sizeof(internal_context), "custom allocator wasn't run properly.");
    TEST_ASSERT_TRUE_MESSAGE(global_default_context.allocators.allocate == context->allocators.allocate, "Wrong allocator.");
    TEST_ASSERT_TRUE_MESSAGE(global_default_context.allocators.deallocate == context->allocators.deallocate, "Wrong deallocator.");
    TEST_ASSERT_TRUE_MESSAGE(global_default_context.allocators.reallocate == context->allocators.reallocate, "Wrong reallocator.");

    custom_deallocator(context, &userdata);
}

static void create_context_should_not_take_incomplete_allocators(void)
{
    cJSON_Allocators allocators1 = {custom_allocator, NULL, NULL};
    cJSON_Allocators allocators2 = {NULL, custom_deallocator, NULL};
    size_t userdata = 0;

    TEST_ASSERT_NULL(cJSON_CreateContext(&allocators1, &userdata));
    TEST_ASSERT_NULL(cJSON_CreateContext(&allocators2, &userdata));
}

static void duplicate_context_should_duplicate_a_context(void)
{
    internal_context *context = NULL;

    context = (internal_context*)cJSON_DuplicateContext(&global_context, NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    TEST_ASSERT_EQUAL_MEMORY(&global_context, context, sizeof(internal_context));

    free(context);
}

static void duplicate_context_should_take_custom_allocators(void)
{
    internal_context *context = NULL;
    cJSON_Allocators allocators = {custom_allocator, custom_deallocator, NULL};
    size_t userdata = 0;

    context = (internal_context*)cJSON_DuplicateContext(&global_context, &allocators, &userdata);
    TEST_ASSERT_NOT_NULL(context);
    TEST_ASSERT_EQUAL_MESSAGE(userdata, sizeof(internal_context), "custom allocator wasn't run properly");

    TEST_ASSERT_EQUAL_MEMORY(&global_context, context, sizeof(internal_context));
    free(context);
}

static void duplicate_context_should_not_take_incomplete_allocators(void)
{
    cJSON_Allocators allocators1 = {custom_allocator, NULL, NULL};
    cJSON_Allocators allocators2 = {NULL, custom_deallocator, NULL};
    size_t userdata = 0;

    TEST_ASSERT_NULL(cJSON_DuplicateContext(&global_context, &allocators1, &userdata));
    TEST_ASSERT_NULL(cJSON_DuplicateContext(&global_context, &allocators2, &userdata));
}

static void set_allocators_should_set_allocators(void)
{
    internal_context *context = NULL;
    cJSON_Allocators allocators = {custom_allocator, custom_deallocator, NULL};
    size_t userdata = 0;

    context = (internal_context*)cJSON_CreateContext(&allocators, &userdata);
    TEST_ASSERT_NOT_NULL(context);

    context = (internal_context*)cJSON_SetAllocators(context, allocators);
    TEST_ASSERT_NOT_NULL(context);
    TEST_ASSERT_TRUE_MESSAGE(custom_allocator == context->allocators.allocate, "Wrong allocator.");
    TEST_ASSERT_TRUE_MESSAGE(custom_deallocator == context->allocators.deallocate, "Wrong deallocator.");
    TEST_ASSERT_NULL_MESSAGE(context->allocators.reallocate, "Reallocator is not null");

    custom_deallocator(context, &userdata);
}

static void set_allocators_should_not_set_incomplete_allocators(void)
{
    internal_context *context = NULL;
    cJSON_Allocators allocators1 = {custom_allocator, NULL, NULL};
    cJSON_Allocators allocators2 = {NULL, custom_deallocator, NULL};

    context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    TEST_ASSERT_NULL(cJSON_SetAllocators(context, allocators1));
    TEST_ASSERT_NULL(cJSON_SetAllocators(context, allocators2));

    free(context);
}

static void set_userdata_should_set_userdata(void)
{
    internal_context *context = NULL;
    size_t userdata = 0;
    context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    context = (internal_context*)cJSON_SetUserdata(context, &userdata);
    TEST_ASSERT_TRUE_MESSAGE(context->userdata == &userdata, "Userdata is incorrect.");

    free(context);
}

static void get_parse_end_should_get_the_parse_end(void)
{
    internal_context context = global_default_context;
    context.end_position = 42;

    TEST_ASSERT_EQUAL_MESSAGE(cJSON_GetParseEnd(&context), 42, "Failed to get parse end.");
}

static void set_prebuffer_size_should_set_buffer_size(void)
{
    internal_context *context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    context = (internal_context*)cJSON_SetPrebufferSize(context, 1024);
    TEST_ASSERT_NOT_NULL(context);

    TEST_ASSERT_EQUAL_MESSAGE(context->buffer_size, 1024, "Didn't set the buffer size correctly.");

    free(context);
}

static void set_prebuffer_size_should_not_allow_empty_sizes(void)
{
    internal_context *context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    TEST_ASSERT_NULL(cJSON_SetPrebufferSize(context, 0));

    free(context);
}

static void set_format_should_set_format(void)
{
    internal_context *context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    context = (internal_context*)cJSON_SetFormat(context, CJSON_FORMAT_MINIFIED);
    TEST_ASSERT_NOT_NULL(context);
    TEST_ASSERT_FALSE_MESSAGE(context->format, "Failed to set CJSON_FORMAT_MINIFIED.");

    context = (internal_context*)cJSON_SetFormat(context, CJSON_FORMAT_DEFAULT);
    TEST_ASSERT_NOT_NULL(context);
    TEST_ASSERT_TRUE_MESSAGE(context->format, "Failed to set CJSON_FORMAT_DEFAULT.");

    TEST_ASSERT_NULL_MESSAGE(cJSON_SetFormat(context, (cJSON_Format)3), "Failed to detect invalid format.");

    free(context);
}

static void make_case_sensitive_should_change_case_sensitivity(void)
{
    internal_context *context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    context = (internal_context*)cJSON_MakeCaseSensitive(context, false);
    TEST_ASSERT_NOT_NULL(context);

    TEST_ASSERT_FALSE_MESSAGE(context->case_sensitive, "Didn't set the case sensitivity correctly.");

    free(context);
}

static void allow_data_after_json_should_change_allow_data_after_json(void)
{
    internal_context *context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    context = (internal_context*)cJSON_AllowDataAfterJson(context, false);
    TEST_ASSERT_NOT_NULL(context);

    TEST_ASSERT_FALSE_MESSAGE(context->allow_data_after_json, "Didn't set allow_data_after_json property correctly.");

    free(context);
}

static void make_duplicate_recursive_should_make_duplicate_recursive(void)
{
    internal_context *context = (internal_context*)cJSON_CreateContext(NULL, NULL);
    TEST_ASSERT_NOT_NULL(context);

    context = (internal_context*)cJSON_MakeDuplicateRecursive(context, false);
    TEST_ASSERT_NOT_NULL(context);
    TEST_ASSERT_FALSE_MESSAGE(context->duplicate_recursive, "Duplicating is not set correctly.");

    free(context);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(create_context_should_create_a_context);
    RUN_TEST(create_context_should_take_custom_allocators);
    RUN_TEST(create_context_should_not_take_incomplete_allocators);
    RUN_TEST(duplicate_context_should_duplicate_a_context);
    RUN_TEST(duplicate_context_should_take_custom_allocators);
    RUN_TEST(duplicate_context_should_not_take_incomplete_allocators);
    RUN_TEST(set_allocators_should_set_allocators);
    RUN_TEST(set_allocators_should_not_set_incomplete_allocators);
    RUN_TEST(set_userdata_should_set_userdata);
    RUN_TEST(get_parse_end_should_get_the_parse_end);
    RUN_TEST(set_prebuffer_size_should_set_buffer_size);
    RUN_TEST(set_prebuffer_size_should_not_allow_empty_sizes);
    RUN_TEST(set_format_should_set_format);
    RUN_TEST(make_case_sensitive_should_change_case_sensitivity);
    RUN_TEST(allow_data_after_json_should_change_allow_data_after_json);
    RUN_TEST(make_duplicate_recursive_should_make_duplicate_recursive);

    return UNITY_END();
}
