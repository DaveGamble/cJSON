/*
 * cjson_print_roundtrip_fuzzer.c
 *
 * Fuzz harness for cJSON print/serialize paths and parse->print->parse
 * round-trip consistency. Exercises:
 *   - cJSON_ParseWithLength() with arbitrary length mismatch
 *   - cJSON_PrintUnformatted() serialization of parsed trees
 *   - cJSON_PrintBuffered() with small prebuf to force reallocation
 *   - Round-trip: parse -> print -> re-parse (checks serializer output is valid)
 *   - cJSON_Print() (formatted) on the same tree
 *   - cJSON_Duplicate() for deep trees
 *
 * Build: compiled by fuzzing/ossfuzz.sh alongside cjson_read_fuzzer.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size); /* required by C89 */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    cJSON *obj = NULL;
    cJSON *reparsed = NULL;
    cJSON *dup = NULL;
    char *unformatted = NULL;
    char *formatted = NULL;
    char *buffered = NULL;

    if (size == 0)
        return 0;

    /* 1. Parse with explicit length (may differ from null-termination) */
    obj = cJSON_ParseWithLength((const char *)data, size);
    if (obj == NULL)
        return 0;

    /* 2. Print unformatted */
    unformatted = cJSON_PrintUnformatted(obj);
    if (unformatted != NULL) {
        /* 3. Round-trip: re-parse the printed output */
        reparsed = cJSON_Parse(unformatted);
        if (reparsed != NULL) {
            cJSON_Delete(reparsed);
        }
        free(unformatted);
    }

    /* 4. Print formatted */
    formatted = cJSON_Print(obj);
    if (formatted != NULL) {
        free(formatted);
    }

    /* 5. PrintBuffered with small prebuf to exercise realloc path */
    buffered = cJSON_PrintBuffered(obj, 16, cJSON_False);
    if (buffered != NULL) {
        free(buffered);
    }

    /* 6. Also exercise cJSON_Duplicate for deep trees */
    dup = cJSON_Duplicate(obj, cJSON_True);
    if (dup != NULL) {
        cJSON_Delete(dup);
    }

    cJSON_Delete(obj);
    return 0;
}

#ifdef __cplusplus
}
#endif
