#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity/examples/unity_config.h"
#include "unity/src/unity.h"
#include "../cJSON.h"
#include "../cJSON_Utils.h"
#include "common.h"

static int g_alloc_count = 0;
static int g_fail_at = -1;

/* Custom malloc hook to simulate OOM on a specific allocation */
static void* failing_malloc(size_t size)
{
    int idx = g_alloc_count++;
    if (idx == g_fail_at)
    {
        return NULL; /* Simulate allocation failure */
    }
    return malloc(size);
}

void setUp(void)
{
    /* Declare variables at top of block */
    cJSON_Hooks hooks;

    hooks.malloc_fn = failing_malloc;
    hooks.free_fn = free;

    g_alloc_count = 0;
    g_fail_at = -1;

    /* Register custom memory hook */
    cJSON_InitHooks(&hooks);
}

void tearDown(void)
{
    /* Reset hooks back to default */
    cJSON_InitHooks(NULL);
}

static void find_pointer_should_handle_null_on_oom(void)
{
    /* 1. ALL variable declarations MUST be at the very top */
    cJSON *root = NULL;
    cJSON *inner = NULL;
    cJSON *target = NULL;
    char *pointer = NULL;

    /* 2. Executable code starts here */
    root = cJSON_CreateArray();
    inner = cJSON_CreateArray();
    cJSON_AddItemToArray(root, inner);

    target = cJSON_CreateNumber(42);
    cJSON_AddItemToArray(inner, target);

    /* Reset alloc counter right before calling FindPointer */
    g_alloc_count = 0;
    g_fail_at = 1;

    pointer = cJSONUtils_FindPointerFromObjectTo(root, target);

    /* Assertion */
    TEST_ASSERT_NULL(pointer);

    /* Cleanup memory */
    cJSON_Delete(root);
}

int CJSON_CDECL main(void)
{
    UNITY_BEGIN();

    RUN_TEST(find_pointer_should_handle_null_on_oom);

    return UNITY_END();
}
