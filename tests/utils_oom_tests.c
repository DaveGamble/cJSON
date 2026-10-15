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

/*
 * Allocation-failure robustness tests for cJSON_Utils.
 *
 * Every allocation made by cJSON is routed through cJSON_InitHooks, so a
 * failure can be injected at each allocation step in turn.  For every
 * injection point the utility functions must neither crash nor leak: any
 * object they create but cannot attach must be released again.
 *
 * Before the corresponding cJSON_Utils.c fixes these tests crashed
 * (NULL-pointer writes in compose_patch/create_patches/
 * cJSONUtils_FindPointerFromObjectTo) and leaked (merge_patch,
 * generate_merge_patch and apply_patch dropping ownership of an item that
 * cJSON_AddItemToObject() refused).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity/examples/unity_config.h"
#include "unity/src/unity.h"
#include "common.h"
#include "../cJSON.h"
#include "../cJSON_Utils.h"

static size_t live_count = 0;
static size_t alloc_seq = 0;
static size_t fail_at = 0;

static void *failing_malloc(size_t size)
{
    void *memory = NULL;

    if (size == 0)
    {
        return NULL;
    }

    alloc_seq++;
    if ((fail_at != 0) && (alloc_seq == fail_at))
    {
        return NULL;
    }

    memory = malloc(size);
    if (memory != NULL)
    {
        live_count++;
    }

    return memory;
}

static void counting_free(void *object)
{
    if (object == NULL)
    {
        return;
    }
    TEST_ASSERT_TRUE(live_count > 0);
    live_count--;
    free(object);
}

static void install_hooks(void)
{
    static cJSON_Hooks hooks;
    hooks.malloc_fn = failing_malloc;
    hooks.free_fn = counting_free;
    cJSON_InitHooks(&hooks);
}

static void set_failure_point(size_t failure)
{
    alloc_seq = 0;
    fail_at = failure;
}

/* Try to delete whatever the calls below returned. */
static void cleanup_utils_results(cJSON *from, cJSON *to, cJSON *patches,
                                  cJSON *target, cJSON *patch)
{
    cJSON_Delete(target);
    cJSON_Delete(patch);
    cJSON_Delete(patches);
    cJSON_Delete(from);
    cJSON_Delete(to);
    TEST_ASSERT_TRUE(live_count == 0);
}

static void generated_patches_should_survive_oom(void)
{
    size_t failure = 0;

    for (failure = 0; failure < 120; failure++)
    {
        cJSON *from = NULL;
        cJSON *to = NULL;
        cJSON *patches = NULL;
        cJSON *target = NULL;

        set_failure_point(failure);
        from = cJSON_Parse("{\"a\":1,\"b\":[1,2,3],\"c\":{\"d\":\"x\"},\"e\":null}");
        to = cJSON_Parse("{\"a\":2,\"b\":[1,9,3,4],\"c\":{\"d\":\"y\"},\"f\":\"new\"}");
        patches = cJSONUtils_GeneratePatches(from, to);
        if (patches != NULL)
        {
            target = cJSON_Duplicate(from, 1);
            if (target != NULL)
            {
                (void)cJSONUtils_ApplyPatches(target, patches);
            }
        }
        cleanup_utils_results(from, to, patches, target, NULL);
    }
}

static void merge_patch_should_survive_oom(void)
{
    size_t failure = 0;

    for (failure = 0; failure < 120; failure++)
    {
        cJSON *target = NULL;
        cJSON *patch = NULL;
        cJSON *merged = NULL;

        set_failure_point(failure);
        target = cJSON_Parse("{\"a\":1,\"b\":{\"c\":2,\"d\":3},\"e\":[1,2]}");
        patch = cJSON_Parse("{\"a\":null,\"b\":{\"c\":9,\"x\":\"y\"},\"e\":[3],\"g\":4}");
        if ((target != NULL) && (patch != NULL))
        {
            /* modifies target in place (or replaces it); ownership moves */
            merged = cJSONUtils_MergePatch(target, patch);
            target = NULL;
        }
        cleanup_utils_results(target, patch, NULL, merged, NULL);
    }
}

static void generated_merge_patch_should_survive_oom(void)
{
    size_t failure = 0;

    for (failure = 0; failure < 120; failure++)
    {
        cJSON *from = NULL;
        cJSON *to = NULL;
        cJSON *patch = NULL;

        set_failure_point(failure);
        from = cJSON_Parse("{\"a\":1,\"b\":{\"c\":2},\"z\":true,\"q\":[1,2]}");
        to = cJSON_Parse("{\"a\":5,\"b\":{\"c\":2,\"d\":4},\"y\":false,\"q\":[1,3]}");
        patch = cJSONUtils_GenerateMergePatch(from, to);
        cleanup_utils_results(from, to, NULL, NULL, patch);
    }
}

static void find_pointer_should_survive_oom(void)
{
    size_t failure = 0;

    for (failure = 0; failure < 120; failure++)
    {
        cJSON *object = NULL;
        cJSON *target = NULL;
        char *pointer = NULL;

        set_failure_point(failure);
        object = cJSON_Parse(
            "{\"a\":{\"b\":[10,20,{\"c\":\"deep\"}]},\"n\":null,\"z\":1}");
        if (object != NULL)
        {
            target = cJSONUtils_GetPointer(object, "/a/b/2/c");
            pointer = cJSONUtils_FindPointerFromObjectTo(object, target);
            cJSON_free(pointer);
        }
        cleanup_utils_results(object, NULL, NULL, NULL, NULL);
    }
}

static void utilities_should_work_without_oom(void)
{
    cJSON *from = cJSON_Parse("{\"a\":1,\"b\":[1,2,3]}");
    cJSON *to = cJSON_Parse("{\"a\":2,\"b\":[1,2,3,4]}");
    cJSON *patches = NULL;
    cJSON *target = NULL;

    set_failure_point(0);
    TEST_ASSERT_NOT_NULL(from);
    TEST_ASSERT_NOT_NULL(to);

    patches = cJSONUtils_GeneratePatches(from, to);
    TEST_ASSERT_NOT_NULL(patches);
    target = cJSON_Duplicate(from, 1);
    TEST_ASSERT_NOT_NULL(target);
    TEST_ASSERT_EQUAL_INT(0, cJSONUtils_ApplyPatches(target, patches));

    cleanup_utils_results(from, to, patches, target, NULL);
}

int main(void)
{
    install_hooks();

    UNITY_BEGIN();

    RUN_TEST(utilities_should_work_without_oom);
    RUN_TEST(generated_patches_should_survive_oom);
    RUN_TEST(merge_patch_should_survive_oom);
    RUN_TEST(generated_merge_patch_should_survive_oom);
    RUN_TEST(find_pointer_should_survive_oom);

    return UNITY_END();
}
