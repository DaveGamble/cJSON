#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "unity/examples/unity_config.h"
#include "unity/src/unity.h"

static void parse_memory_should_parse_true(void) {
    cJSON *item;
    const char *mem =
                   "{"
                   "\"hello\":\"world\""
                   "}",
               *end = mem + strlen(mem);
    item = cJSON_ParseWithOpts(mem, &end, false);
    TEST_ASSERT_NOT_NULL(item);
    cJSON_Delete(item);
}

int main(void) {
    /* initialize cJSON item */
    UNITY_BEGIN();
    RUN_TEST(parse_memory_should_parse_true);
    return UNITY_END();
}
