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

/* These are unit tests for the cJSON_saveJSONfile &  cJSON_loadJSONfile functions */
#include "unity/examples/unity_config.h"
#include "unity/src/unity.h"
#include "common.h"

#define TEST_VALUE "test value"

static void save_file_should_succeed(void)
{
    char *errorMessage = NULL;
    cJSON *object;
    int rc = 0;

    errorMessage = (char *)malloc(512);
    object = cJSON_CreateObject();
    cJSON_AddNumberToObject(object, TEST_VALUE, 99);

    rc = cJSON_saveJSONfile("./testfile", object, errorMessage);
    TEST_ASSERT_EQUAL(1, rc); 
    free(errorMessage);
}

static void save_file_with_bad_path(void)
{
    char *errorMessage = NULL;
    cJSON *object;
    int rc = 0;

    errorMessage = (char *)malloc(512);
    object = cJSON_CreateObject();

    rc = cJSON_saveJSONfile("/!@#$!this doesn't exist/testfile", object, errorMessage);
    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL_STRING("Cannot open the file '/!@#$!this doesn't exist/testfile' to write.", errorMessage);
    free(errorMessage);
}

static void save_file_with_bad_path_no_error_msg(void)
{
    char *errorMessage = NULL;
    cJSON *object;
    int rc = 0;

    object = cJSON_CreateObject();
   
    rc = cJSON_saveJSONfile("/!@#$!this doesn't exist/testfile", object, errorMessage);
    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL_STRING(NULL, errorMessage);
    free(errorMessage);
    cJSON_Delete(object);
}

static void save_file_with_NULL_object(void)
{
    char *errorMessage = NULL;
    int rc = 0;

    errorMessage = (char *)malloc(512);
    rc = cJSON_saveJSONfile("./testfile", NULL, errorMessage);
    TEST_ASSERT_EQUAL(0, rc);
    free(errorMessage);
}

static void load_file_successs_preexisting_object(void)
{
    char *errorMessage = NULL;
    cJSON *object = NULL;
    int rc = 0;
    double testValue = 0;
    cJSON *numberJson = NULL;

    object = cJSON_CreateObject();
  
    errorMessage = (char *)malloc(512);
    rc = cJSON_loadJSONfile("./testfile", &object, errorMessage);

    numberJson = cJSON_GetObjectItem(object, TEST_VALUE);
    testValue = cJSON_GetNumberValue(numberJson);

    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_EQUAL(99, testValue);
    free(errorMessage);
    cJSON_Delete(object);
}

static void load_file_success(void)
{
    char *errorMessage = NULL;
    cJSON *object = NULL;
    int rc = 0;
    double testValue = 0;
    cJSON *numberJson = NULL;

    errorMessage = (char *)malloc(512);
    rc = cJSON_loadJSONfile("./testfile", &object, errorMessage);
    
    numberJson = cJSON_GetObjectItem(object, TEST_VALUE);
    testValue = cJSON_GetNumberValue(numberJson);
    
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_EQUAL(99, testValue);
    free(errorMessage);
}

static void load_file_bad_filename(void)
{
    char *errorMessage = NULL;
    cJSON *object = NULL;
    int rc = 0;

    errorMessage = (char *)malloc(512);
    rc = cJSON_loadJSONfile("./notfound", &object, errorMessage);
    TEST_ASSERT_EQUAL_STRING("Cannot open the file './notfound' to read.",errorMessage);
    TEST_ASSERT_EQUAL(0, rc);
    free(errorMessage);
}

static void load_file_NULL_filename(void)
{
    char *errorMessage = NULL;
    cJSON *object = NULL;
    int rc = 0;

    errorMessage = (char *)malloc(512);
    rc = cJSON_loadJSONfile(NULL, &object, errorMessage);
    
    TEST_ASSERT_EQUAL(-3, rc);
    TEST_ASSERT_EQUAL_STRING("The filename was NULL",errorMessage);
    free(errorMessage);
}

static void load_file_NULL_object(void)
{
    char *errorMessage = NULL;
    int rc = 0;

    errorMessage = (char *)malloc(512);
    rc = cJSON_loadJSONfile("./testfile", NULL, errorMessage);
    
    TEST_ASSERT_EQUAL(-4, rc);
    free(errorMessage);
}

int main(void)
{
    /* initialize cJSON item */
    UNITY_BEGIN();
  
    RUN_TEST(save_file_with_bad_path);
    RUN_TEST(save_file_with_bad_path_no_error_msg);
    RUN_TEST(save_file_with_NULL_object);
    RUN_TEST(save_file_should_succeed);

    RUN_TEST(load_file_success);
    RUN_TEST(load_file_successs_preexisting_object);
    RUN_TEST(load_file_bad_filename);
    RUN_TEST(load_file_NULL_filename);
    RUN_TEST(load_file_NULL_object);

    return UNITY_END();
}
