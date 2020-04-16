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
#include "../cJSON_Utils.h"

static int cmp(const double *a, const double *b) {
    if (*a > *b)
    {
        return -1;
    }
    if (*a < *b)
    {
        return 1;
    }
    return 0;
}

static int jcmp_double(const cJSON *a, const cJSON *b)
{
    return cmp(&(a->valuedouble), &(b->valuedouble));
}

static void cjson_utils_sort_null(void)
{
    cJSON *empty = NULL;
    cJSONUtils_SortArray(empty, jcmp_double);
    TEST_ASSERT_NULL(empty);
}

static void cjson_utils_sort_empty(void)
{
    cJSON *array = cJSON_CreateArray();
    cJSONUtils_SortArray(array, jcmp_double);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(array));
    cJSON_Delete(array);
}

static void cjson_utils_sort_size1(void)
{
    double d[1] = {0.0};
    cJSON *array = cJSON_CreateDoubleArray(d, 1);
    cJSONUtils_SortArray(array, jcmp_double);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    cJSON_Delete(array);
}

static void cjson_utils_sort_double_issorted(void)
{
    size_t size = 8192;
    size_t i;
    cJSON *array, *elt, *prev;
    double *d;

    /* generate array with rand doubles */
    d = (double*)malloc(size * sizeof(double));
    for (i = 0; i < size; i++)
    {
        d[i] = (double)rand();
    }
    array = cJSON_CreateDoubleArray(d, (int)size);
    cJSONUtils_SortArray(array, jcmp_double);
    TEST_ASSERT_EQUAL_INT(size, cJSON_GetArraySize(array));

    /* check if order is achieved */
    prev = NULL;
    cJSON_ArrayForEach(elt, array)
    {
        if (prev != NULL) {
            TEST_ASSERT_LESS_THAN_INT(1, jcmp_double(prev, elt));
        }
        prev = elt;
    }
    cJSON_Delete(array);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_utils_sort_null);
    RUN_TEST(cjson_utils_sort_empty);
    RUN_TEST(cjson_utils_sort_size1);
    RUN_TEST(cjson_utils_sort_double_issorted);

    return UNITY_END();
}
