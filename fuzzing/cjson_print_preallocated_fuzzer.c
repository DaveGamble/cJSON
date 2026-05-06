#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

static void fuzz_assert(int condition) {
    if (!condition) {
        abort();
    }
}

static int read_u8(const uint8_t **data, size_t *size, uint8_t *out) {
    if (*size < 1) {
        return 0;
    }

    *out = **data;
    (*data)++;
    (*size)--;
    return 1;
}

static int read_bytes(const uint8_t **data, size_t *size, uint8_t *out, size_t n) {
    if (*size < n) {
        return 0;
    }

    memcpy(out, *data, n);
    (*data) += n;
    (*size) -= n;
    return 1;
}

static int read_double(const uint8_t **data, size_t *size, double *out) {
    union {
        uint64_t u64;
        double d;
    } conv;

    conv.u64 = 0;

    if (*size >= sizeof(uint64_t)) {
        if (!read_bytes(data, size, (uint8_t *)&conv.u64, sizeof(uint64_t))) {
            return 0;
        }
    } else {
        size_t n = *size;

        if (n > 0) {
            memcpy(&conv.u64, *data, n);
            (*data) += n;
            (*size) -= n;
        }
    }

    *out = conv.d;
    return 1;
}

static char *read_bounded_string(const uint8_t **data, size_t *size, size_t max_len) {
    uint8_t raw_len = 0;
    size_t len = 0;
    char *out = NULL;

    if (!read_u8(data, size, &raw_len)) {
        return NULL;
    }

    len = (size_t)(raw_len % (max_len + 1));
    if (*size < len) {
        len = *size;
    }

    out = (char *)malloc(len + 1);
    if (out == NULL) {
        return NULL;
    }

    if (len > 0) {
        memcpy(out, *data, len);
        (*data) += len;
        (*size) -= len;
    }

    out[len] = '\0';
    for (size_t i = 0; i < len; ++i) {
        if (out[i] == '\0') {
            out[i] = 'A';
        }
    }

    return out;
}

static int keys_equal_case_insensitive(const char *left, const char *right) {
    if ((left == NULL) || (right == NULL)) {
        return 0;
    }

    while ((*left != '\0') && (*right != '\0')) {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
            return 0;
        }

        left++;
        right++;
    }

    return (*left == '\0') && (*right == '\0');
}

static cJSON_bool tree_has_ambiguous_object_keys(const cJSON *item) {
    const cJSON *outer = NULL;
    const cJSON *inner = NULL;

    if (item == NULL) {
        return 0;
    }

    if (cJSON_IsObject(item)) {
        for (outer = item->child; outer != NULL; outer = outer->next) {
            if (tree_has_ambiguous_object_keys(outer)) {
                return 1;
            }

            for (inner = outer->next; inner != NULL; inner = inner->next) {
                if (keys_equal_case_insensitive(outer->string, inner->string)) {
                    return 1;
                }
            }
        }

        return 0;
    }

    for (outer = item->child; outer != NULL; outer = outer->next) {
        if (tree_has_ambiguous_object_keys(outer)) {
            return 1;
        }
    }

    return 0;
}

static cJSON_bool tree_contains_raw(const cJSON *item) {
    const cJSON *child = NULL;

    if (item == NULL) {
        return 0;
    }

    if (cJSON_IsRaw(item)) {
        return 1;
    }

    for (child = item->child; child != NULL; child = child->next) {
        if (tree_contains_raw(child)) {
            return 1;
        }
    }

    return 0;
}

static cJSON_bool tree_contains_nonfinite_number(const cJSON *item) {
    const cJSON *child = NULL;

    if (item == NULL) {
        return 0;
    }

    if (cJSON_IsNumber(item) && !isfinite(item->valuedouble)) {
        return 1;
    }

    for (child = item->child; child != NULL; child = child->next) {
        if (tree_contains_nonfinite_number(child)) {
            return 1;
        }
    }

    return 0;
}

static cJSON_bool tree_is_roundtrip_safe(const cJSON *item) {
    return (item != NULL) &&
           !tree_contains_raw(item) &&
           !tree_contains_nonfinite_number(item) &&
           !tree_has_ambiguous_object_keys(item);
}

static cJSON *create_seed_tree(void) {
    cJSON *root = NULL;
    cJSON *nested = NULL;
    cJSON *items = NULL;

    root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    if (cJSON_AddNumberToObject(root, "number", 1.0) == NULL) {
        goto fail;
    }
    if (cJSON_AddStringToObject(root, "text", "seed") == NULL) {
        goto fail;
    }
    if (cJSON_AddBoolToObject(root, "flag", 1) == NULL) {
        goto fail;
    }
    if (cJSON_AddNullToObject(root, "empty") == NULL) {
        goto fail;
    }

    nested = cJSON_AddObjectToObject(root, "nested");
    if (nested == NULL) {
        goto fail;
    }
    if (cJSON_AddStringToObject(nested, "label", "alpha") == NULL) {
        goto fail;
    }
    if (cJSON_AddNumberToObject(nested, "value", 7.0) == NULL) {
        goto fail;
    }

    items = cJSON_AddArrayToObject(root, "items");
    if (items == NULL) {
        goto fail;
    }
    if (!cJSON_AddItemToArray(items, cJSON_CreateNumber(1.0))) {
        goto fail;
    }
    if (!cJSON_AddItemToArray(items, cJSON_CreateString("x"))) {
        goto fail;
    }
    if (!cJSON_AddItemToArray(items, cJSON_CreateFalse())) {
        goto fail;
    }

    return root;

fail:
    cJSON_Delete(root);
    return NULL;
}

static cJSON *create_scalar_item(const uint8_t **data, size_t *size) {
    uint8_t kind = 0;
    char *payload = NULL;
    double number = 0.0;
    cJSON *item = NULL;

    if (!read_u8(data, size, &kind)) {
        return cJSON_CreateNull();
    }

    switch (kind % 5) {
        case 0:
            (void)read_double(data, size, &number);
            return cJSON_CreateNumber(number);

        case 1:
            payload = read_bounded_string(data, size, 32);
            if (payload == NULL) {
                return cJSON_CreateString("");
            }
            item = cJSON_CreateString(payload);
            free(payload);
            return item;

        case 2:
            return cJSON_CreateTrue();

        case 3:
            return cJSON_CreateFalse();

        case 4:
        default:
            return cJSON_CreateNull();
    }
}

static cJSON *create_raw_item(uint8_t selector) {
    static const char *raw_values[] = {
        "null",
        "true",
        "false",
        "0",
        "\"raw\"",
        "[]",
        "{}"
    };

    return cJSON_CreateRaw(raw_values[selector % (sizeof(raw_values) / sizeof(raw_values[0]))]);
}

static cJSON *create_number_item(const uint8_t **data, size_t *size) {
    double number = 0.0;

    (void)read_double(data, size, &number);
    return cJSON_CreateNumber(number);
}

static void verify_invalid_argument_behavior(cJSON *item) {
    char buffer[8];

    memset(buffer, 'Z', sizeof(buffer));

    fuzz_assert(!cJSON_PrintPreallocated(NULL, buffer, (int)sizeof(buffer), 0));
    fuzz_assert(!cJSON_PrintPreallocated(NULL, buffer, (int)sizeof(buffer), 1));
    fuzz_assert(!cJSON_PrintPreallocated(item, NULL, 1, 0));
    fuzz_assert(!cJSON_PrintPreallocated(item, NULL, 1, 1));
    fuzz_assert(!cJSON_PrintPreallocated(item, buffer, -1, 0));
    fuzz_assert(!cJSON_PrintPreallocated(item, buffer, -1, 1));
}

static void verify_roundtrip_from_text(const cJSON *item, const char *text) {
    cJSON *parsed = NULL;

    fuzz_assert(item != NULL);
    fuzz_assert(text != NULL);

    if (!tree_is_roundtrip_safe(item)) {
        return;
    }

    parsed = cJSON_Parse(text);
    fuzz_assert(parsed != NULL);
    fuzz_assert(cJSON_Compare(item, parsed, 1));
    fuzz_assert(cJSON_Compare(item, parsed, 0));
    cJSON_Delete(parsed);
}

static void verify_success_case(cJSON *item, cJSON_bool format, size_t buffer_length) {
    char *expected = NULL;
    char *after = NULL;
    unsigned char *buffer = NULL;
    size_t expected_len = 0;
    size_t total_len = 0;
    const size_t redzone = 32;
    cJSON_bool result = 0;

    fuzz_assert(item != NULL);

    expected = format ? cJSON_Print(item) : cJSON_PrintUnformatted(item);
    if (expected == NULL) {
        return;
    }

    expected_len = strlen(expected);
    fuzz_assert(buffer_length >= (expected_len + 1));

    total_len = buffer_length + redzone;
    buffer = (unsigned char *)malloc(total_len);
    if (buffer == NULL) {
        free(expected);
        return;
    }

    memset(buffer, 0xA5, total_len);

    result = cJSON_PrintPreallocated(item, (char *)buffer, (int)buffer_length, format);
    fuzz_assert(result);
    fuzz_assert(strcmp((const char *)buffer, expected) == 0);
    fuzz_assert(buffer[expected_len] == '\0');

    for (size_t i = buffer_length; i < total_len; ++i) {
        fuzz_assert(buffer[i] == 0xA5);
    }

    after = format ? cJSON_Print(item) : cJSON_PrintUnformatted(item);
    fuzz_assert(after != NULL);
    fuzz_assert(strcmp(expected, after) == 0);
    verify_roundtrip_from_text(item, (const char *)buffer);

    free(after);
    free(buffer);
    free(expected);
}

static void verify_too_small_failure(cJSON *item, cJSON_bool format) {
    char *expected = NULL;
    char *after = NULL;
    unsigned char *buffer = NULL;
    size_t expected_len = 0;
    size_t buffer_length = 0;
    size_t total_len = 0;
    const size_t redzone = 32;
    cJSON_bool result = 0;

    fuzz_assert(item != NULL);

    expected = format ? cJSON_Print(item) : cJSON_PrintUnformatted(item);
    if (expected == NULL) {
        return;
    }

    expected_len = strlen(expected);
    if (expected_len == 0) {
        free(expected);
        return;
    }

    buffer_length = expected_len;
    total_len = buffer_length + redzone;
    buffer = (unsigned char *)malloc(total_len);
    if (buffer == NULL) {
        free(expected);
        return;
    }

    memset(buffer, 0x5A, total_len);
    result = cJSON_PrintPreallocated(item, (char *)buffer, (int)buffer_length, format);
    fuzz_assert(!result);

    for (size_t i = buffer_length; i < total_len; ++i) {
        fuzz_assert(buffer[i] == 0x5A);
    }

    after = format ? cJSON_Print(item) : cJSON_PrintUnformatted(item);
    fuzz_assert(after != NULL);
    fuzz_assert(strcmp(expected, after) == 0);

    free(after);
    free(buffer);
    free(expected);
}

static void verify_zero_length_failure(cJSON *item, cJSON_bool format) {
    unsigned char buffer[32];

    fuzz_assert(item != NULL);

    memset(buffer, 0xCC, sizeof(buffer));
    fuzz_assert(!cJSON_PrintPreallocated(item, (char *)buffer, 0, format));
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        fuzz_assert(buffer[i] == 0xCC);
    }
}

static void verify_exact_output_oracles(cJSON *item) {
    char *formatted = NULL;
    char *unformatted = NULL;

    fuzz_assert(item != NULL);

    verify_invalid_argument_behavior(item);

    formatted = cJSON_Print(item);
    unformatted = cJSON_PrintUnformatted(item);
    if ((formatted == NULL) || (unformatted == NULL)) {
        free(formatted);
        free(unformatted);
        return;
    }

    verify_success_case(item, 0, strlen(unformatted) + 5);
    verify_success_case(item, 0, strlen(unformatted) + 17);
    verify_success_case(item, 1, strlen(formatted) + 5);
    verify_success_case(item, 1, strlen(formatted) + 17);

    verify_too_small_failure(item, 0);
    verify_too_small_failure(item, 1);
    verify_zero_length_failure(item, 0);
    verify_zero_length_failure(item, 1);

    free(formatted);
    free(unformatted);
}

static void run_seed_tree_scenario(void) {
    cJSON *root = create_seed_tree();

    if (root == NULL) {
        return;
    }

    verify_exact_output_oracles(root);
    cJSON_Delete(root);
}

static void run_scalar_scenario(const uint8_t **data, size_t *size) {
    cJSON *item = create_scalar_item(data, size);

    if (item == NULL) {
        return;
    }

    verify_exact_output_oracles(item);
    cJSON_Delete(item);
}

static void run_raw_scenario(uint8_t selector) {
    cJSON *item = create_raw_item(selector);

    if (item == NULL) {
        return;
    }

    verify_exact_output_oracles(item);
    cJSON_Delete(item);
}

static void run_number_scenario(const uint8_t **data, size_t *size) {
    cJSON *item = create_number_item(data, size);

    if (item == NULL) {
        return;
    }

    verify_exact_output_oracles(item);
    cJSON_Delete(item);
}

static void run_parsed_tree_scenario(const uint8_t *data, size_t size) {
    cJSON *root = NULL;
    char *input = NULL;

    if ((data == NULL) || (size == 0)) {
        return;
    }

    input = (char *)malloc(size + 1);
    if (input == NULL) {
        return;
    }

    memcpy(input, data, size);
    input[size] = '\0';

    root = cJSON_Parse(input);
    free(input);

    if (root == NULL) {
        return;
    }

    verify_exact_output_oracles(root);
    cJSON_Delete(root);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    const uint8_t *payload = NULL;
    size_t payload_size = 0;
    uint8_t raw_cases = 0;
    size_t case_count = 0;

    if ((data == NULL) || (size == 0)) {
        return 0;
    }

    if (!read_u8(&data, &size, &raw_cases)) {
        return 0;
    }

    payload = data;
    payload_size = size;
    case_count = (size_t)(raw_cases % 20);

    for (size_t i = 0; (i < case_count) && (size > 0); ++i) {
        uint8_t opcode = 0;
        uint8_t selector = 0;

        if (!read_u8(&data, &size, &opcode)) {
            break;
        }

        switch (opcode % 4) {
            case 0:
                run_seed_tree_scenario();
                break;

            case 1:
                run_scalar_scenario(&data, &size);
                break;

            case 2:
                run_number_scenario(&data, &size);
                break;

            case 3:
            default:
                (void)read_u8(&data, &size, &selector);
                run_raw_scenario(selector);
                break;
        }
    }

    if (payload_size > 0) {
        run_parsed_tree_scenario(payload, payload_size);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
