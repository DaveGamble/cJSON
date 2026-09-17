/*
 * cjson_mutate_fuzzer.c
 *
 * Fuzz target for cJSON mutation, duplication, and comparison APIs.
 * The existing cjson_read_fuzzer covers Parse/Print/Minify only.
 * This harness exercises the following uncovered paths:
 *
 *   cJSON_Duplicate()               - recursive deep copy
 *   cJSON_Compare()                 - deep equality comparison
 *   cJSON_InsertItemInArray()       - mid-array insertion (shifts items)
 *   cJSON_DetachItemFromArray()     - detach + re-insert
 *   cJSON_ReplaceItemViaPointer()   - in-place item replacement
 *   cJSON_AddItemToArray()          - append to array
 *   cJSON_ParseWithLengthOpts()     - length-bounded parser variant
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../cJSON.h"

/* Consume one byte from the fuzz buffer */
static uint8_t consume_u8(const uint8_t **data, size_t *size)
{
    if (*size == 0) return 0;
    uint8_t v = **data; (*data)++; (*size)--; return v;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 4) return 0;

    /* First byte selects which operation sequence to run */
    uint8_t op = consume_u8(&data, &size);

    /* Remaining bytes are the JSON string (may or may not be NUL-terminated) */
    /* ParseWithLengthOpts accepts non-NUL-terminated input */
    cJSON *root = cJSON_ParseWithLengthOpts(
        (const char *)data, size, NULL, /*require_null_terminated=*/0);
    if (root == NULL) return 0;

    switch (op % 6) {
    case 0: {
        /* Duplicate (deep copy) then compare for equality */
        cJSON *copy = cJSON_Duplicate(root, /*recurse=*/1);
        if (copy != NULL) {
            cJSON_Compare(root, copy, /*case_sensitive=*/1);
            cJSON_Delete(copy);
        }
        break;
    }
    case 1: {
        /* Build an array, insert items at various positions */
        cJSON *arr = cJSON_CreateArray();
        if (arr != NULL) {
            /* Append a duplicate of root */
            cJSON *item0 = cJSON_Duplicate(root, 1);
            if (item0) cJSON_AddItemToArray(arr, item0);

            /* Insert at position 0 (shifts existing) */
            cJSON *item1 = cJSON_CreateString("inserted");
            if (item1) cJSON_InsertItemInArray(arr, 0, item1);

            /* Detach the first element */
            cJSON *detached = cJSON_DetachItemFromArray(arr, 0);
            if (detached) cJSON_Delete(detached);

            cJSON_Delete(arr);
        }
        break;
    }
    case 2: {
        /* ReplaceItemViaPointer on the first child of root */
        if (root->child != NULL) {
            cJSON *replacement = cJSON_CreateNumber(42.0);
            if (replacement) {
                /* ReplaceItemViaPointer transfers ownership on success */
                if (!cJSON_ReplaceItemViaPointer(root, root->child, replacement))
                    cJSON_Delete(replacement);
            }
        }
        break;
    }
    case 3: {
        /* ParseWithLengthOpts with a deliberately short length */
        size_t short_len = size / 2;
        cJSON *partial = cJSON_ParseWithLengthOpts(
            (const char *)data, short_len, NULL, 0);
        if (partial) cJSON_Delete(partial);
        break;
    }
    case 4: {
        /* Duplicate with recurse=0 (shallow) then print */
        cJSON *shallow = cJSON_Duplicate(root, /*recurse=*/0);
        if (shallow) {
            char *printed = cJSON_PrintUnformatted(shallow);
            if (printed) free(printed);
            cJSON_Delete(shallow);
        }
        break;
    }
    case 5: {
        /* Compare root against itself (must return true) */
        cJSON_Compare(root, root, /*case_sensitive=*/0);
        cJSON_Compare(root, root, /*case_sensitive=*/1);
        break;
    }
    }

    cJSON_Delete(root);
    return 0;
}
