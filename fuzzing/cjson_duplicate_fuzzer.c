#include <ctype.h>
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

static int choose_distinct_int_value(int old_value) {
    return (old_value == 0) ? 1 : 0;
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

static cJSON *create_seed_tree(void) {
    cJSON *root = NULL;
    cJSON *nested = NULL;
    cJSON *items = NULL;
    cJSON *const_item = NULL;

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

    nested = cJSON_AddObjectToObject(root, "nested");
    if (nested == NULL) {
        goto fail;
    }
    if (cJSON_AddNumberToObject(nested, "value", 7.0) == NULL) {
        goto fail;
    }
    if (cJSON_AddStringToObject(nested, "label", "alpha") == NULL) {
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

    const_item = cJSON_CreateNumber(9.0);
    if ((const_item == NULL) || !cJSON_AddItemToObjectCS(root, "constKey", const_item)) {
        cJSON_Delete(const_item);
        goto fail;
    }

    return root;

fail:
    cJSON_Delete(root);
    return NULL;
}

static cJSON *seed_nested_value(cJSON *root) {
    cJSON *nested = NULL;

    fuzz_assert(root != NULL);
    nested = cJSON_GetObjectItemCaseSensitive(root, "nested");
    fuzz_assert(nested != NULL);
    return cJSON_GetObjectItemCaseSensitive(nested, "value");
}

static cJSON *seed_nested_label(cJSON *root) {
    cJSON *nested = NULL;

    fuzz_assert(root != NULL);
    nested = cJSON_GetObjectItemCaseSensitive(root, "nested");
    fuzz_assert(nested != NULL);
    return cJSON_GetObjectItemCaseSensitive(nested, "label");
}

static void verify_roundtrip_if_safe(const cJSON *item) {
    char *printed = NULL;
    cJSON *parsed = NULL;

    if ((item == NULL) || tree_has_ambiguous_object_keys(item)) {
        return;
    }

    printed = cJSON_PrintUnformatted(item);
    if (printed == NULL) {
        return;
    }

    parsed = cJSON_Parse(printed);
    fuzz_assert(parsed != NULL);
    fuzz_assert(cJSON_Compare(item, parsed, 1));
    fuzz_assert(cJSON_Compare(item, parsed, 0));
    cJSON_Delete(parsed);
    free(printed);
}

static void expect_printed_text(const cJSON *item, const char *expected) {
    char *printed = NULL;

    fuzz_assert(item != NULL);
    fuzz_assert(expected != NULL);

    printed = cJSON_PrintUnformatted(item);
    fuzz_assert(printed != NULL);
    fuzz_assert(strcmp(printed, expected) == 0);
    free(printed);
}

static void verify_null_duplicate_behavior(void) {
    fuzz_assert(cJSON_Duplicate(NULL, 0) == NULL);
    fuzz_assert(cJSON_Duplicate(NULL, 1) == NULL);
}

static void verify_top_level_duplicate_metadata(const cJSON *original,
                                                const cJSON *dup,
                                                cJSON_bool recurse) {
    fuzz_assert(original != NULL);
    fuzz_assert(dup != NULL);
    fuzz_assert(original != dup);
    fuzz_assert(dup->next == NULL);
    fuzz_assert(dup->prev == NULL);
    fuzz_assert((dup->type & cJSON_IsReference) == 0);
    fuzz_assert((dup->type & 0xFF) == (original->type & 0xFF));

    if (original->valuestring != NULL) {
        fuzz_assert(dup->valuestring != NULL);
        fuzz_assert(strcmp(original->valuestring, dup->valuestring) == 0);
        fuzz_assert(original->valuestring != dup->valuestring);
    } else {
        fuzz_assert(dup->valuestring == NULL);
    }

    if (original->string != NULL) {
        fuzz_assert(dup->string != NULL);
        fuzz_assert(strcmp(original->string, dup->string) == 0);
        if ((original->type & cJSON_StringIsConst) != 0) {
            fuzz_assert((dup->type & cJSON_StringIsConst) != 0);
            fuzz_assert(original->string == dup->string);
        } else {
            fuzz_assert((dup->type & cJSON_StringIsConst) == 0);
            fuzz_assert(original->string != dup->string);
        }
    } else {
        fuzz_assert(dup->string == NULL);
    }

    if (cJSON_IsNumber(original)) {
        fuzz_assert(dup->valueint == original->valueint);
    }

    if (recurse) {
        if (original->child != NULL) {
            fuzz_assert(dup->child != NULL);
            fuzz_assert(dup->child != original->child);
        } else {
            fuzz_assert(dup->child == NULL);
        }
    } else {
        fuzz_assert(dup->child == NULL);
    }
}

static void verify_tree_distinct(const cJSON *original, const cJSON *dup) {
    fuzz_assert(original != NULL);
    fuzz_assert(dup != NULL);
    fuzz_assert(original != dup);
    fuzz_assert((dup->type & cJSON_IsReference) == 0);
    fuzz_assert((dup->type & 0xFF) == (original->type & 0xFF));

    if (original->valuestring != NULL) {
        fuzz_assert(dup->valuestring != NULL);
        fuzz_assert(strcmp(original->valuestring, dup->valuestring) == 0);
        fuzz_assert(original->valuestring != dup->valuestring);
    } else {
        fuzz_assert(dup->valuestring == NULL);
    }

    if (original->string != NULL) {
        fuzz_assert(dup->string != NULL);
        fuzz_assert(strcmp(original->string, dup->string) == 0);
        if ((original->type & cJSON_StringIsConst) != 0) {
            fuzz_assert((dup->type & cJSON_StringIsConst) != 0);
            fuzz_assert(original->string == dup->string);
        } else {
            fuzz_assert((dup->type & cJSON_StringIsConst) == 0);
            fuzz_assert(original->string != dup->string);
        }
    } else {
        fuzz_assert(dup->string == NULL);
    }

    if (cJSON_IsNumber(original)) {
        fuzz_assert(dup->valueint == original->valueint);
    }

    if (original->child != NULL) {
        fuzz_assert(dup->child != NULL);
        fuzz_assert(original->child != dup->child);
        verify_tree_distinct(original->child, dup->child);
    } else {
        fuzz_assert(dup->child == NULL);
    }

    if (original->next != NULL) {
        fuzz_assert(dup->next != NULL);
        fuzz_assert(original->next != dup->next);
        verify_tree_distinct(original->next, dup->next);
    } else {
        fuzz_assert(dup->next == NULL);
    }
}

static int mutate_first_number_or_string(const cJSON *original, cJSON *dup) {
    cJSON *string_item = NULL;
    const char *replacement = NULL;
    int old_value = 0;
    int new_value = 0;
    const char *old_text = NULL;

    fuzz_assert(original != NULL);
    fuzz_assert(dup != NULL);

    if (cJSON_IsNumber(original) && cJSON_IsNumber(dup)) {
        old_value = original->valueint;
        new_value = choose_distinct_int_value(old_value);

        cJSON_SetNumberValue(dup, (double)new_value);
        fuzz_assert(original->valueint == old_value);
        fuzz_assert(dup->valueint == new_value);
        return 1;
    }

    if (cJSON_IsString(original) && cJSON_IsString(dup)) {
        string_item = dup;
        old_text = original->valuestring;
        replacement = (strcmp(old_text != NULL ? old_text : "", "changed") == 0) ? "changed!" : "changed";

        fuzz_assert(cJSON_SetValuestring(string_item, replacement) != NULL);
        fuzz_assert(strcmp(original->valuestring, old_text) == 0);
        fuzz_assert(strcmp(string_item->valuestring, replacement) == 0);
        return 1;
    }

    if ((original->child != NULL) && (dup->child != NULL)) {
        if (mutate_first_number_or_string(original->child, dup->child)) {
            return 1;
        }
    }

    if ((original->next != NULL) && (dup->next != NULL)) {
        if (mutate_first_number_or_string(original->next, dup->next)) {
            return 1;
        }
    }

    return 0;
}

static void run_recursive_seed_scenario(const uint8_t **data, size_t *size) {
    cJSON *root = NULL;
    cJSON *dup = NULL;
    cJSON *dup2 = NULL;
    cJSON *root_number = NULL;
    cJSON *dup_number = NULL;
    cJSON *root_label = NULL;
    cJSON *dup2_label = NULL;
    char *replacement = NULL;
    int root_value_before = 0;
    int mutated_value = 0;

    root = create_seed_tree();
    if (root == NULL) {
        return;
    }

    dup = cJSON_Duplicate(root, 1);
    if (dup == NULL) {
        cJSON_Delete(root);
        return;
    }

    verify_top_level_duplicate_metadata(root, dup, 1);
    verify_tree_distinct(root, dup);
    fuzz_assert(cJSON_Compare(root, dup, 1));
    fuzz_assert(cJSON_Compare(root, dup, 0));
    verify_roundtrip_if_safe(root);
    verify_roundtrip_if_safe(dup);

    replacement = read_bounded_string(data, size, 24);
    if (replacement == NULL) {
        replacement = (char *)malloc(sizeof("mutated"));
        if (replacement != NULL) {
            memcpy(replacement, "mutated", sizeof("mutated"));
        }
    }
    if (replacement == NULL) {
        cJSON_Delete(dup);
        cJSON_Delete(root);
        return;
    }

    if (strcmp(replacement, "alpha") == 0) {
        char *tmp = (char *)malloc(sizeof("alpha!"));
        if (tmp != NULL) {
            memcpy(tmp, "alpha!", sizeof("alpha!"));
            free(replacement);
            replacement = tmp;
        }
    }

    root_number = seed_nested_value(root);
    dup_number = seed_nested_value(dup);
    root_label = seed_nested_label(root);

    root_value_before = root_number->valueint;
    mutated_value = choose_distinct_int_value(root_value_before);
    cJSON_SetNumberValue(dup_number, (double)mutated_value);
    fuzz_assert(root_number->valueint == root_value_before);
    fuzz_assert(cJSON_SetValuestring(dup ? seed_nested_label(dup) : NULL, replacement) != NULL);
    fuzz_assert(strcmp(root_label->valuestring, "alpha") == 0);
    fuzz_assert(!cJSON_Compare(root, dup, 1));
    fuzz_assert(!cJSON_Compare(root, dup, 0));

    cJSON_Delete(dup);
    dup = NULL;

    dup2 = cJSON_Duplicate(root, 1);
    if (dup2 != NULL) {
        verify_top_level_duplicate_metadata(root, dup2, 1);
        verify_tree_distinct(root, dup2);
        fuzz_assert(cJSON_Compare(root, dup2, 1));
        cJSON_SetNumberValue(seed_nested_value(root), (double)mutated_value);
        fuzz_assert(seed_nested_value(dup2)->valueint == root_value_before);
        fuzz_assert(!cJSON_Compare(root, dup2, 1));

        dup2_label = seed_nested_label(dup2);
        fuzz_assert(strcmp(dup2_label->valuestring, "alpha") == 0);
    }

    free(replacement);
    cJSON_Delete(dup2);
    cJSON_Delete(root);
}

static void run_nonrecursive_seed_scenario(void) {
    cJSON *root = NULL;
    cJSON *dup = NULL;

    root = create_seed_tree();
    if (root == NULL) {
        return;
    }

    dup = cJSON_Duplicate(root, 0);
    if (dup == NULL) {
        cJSON_Delete(root);
        return;
    }

    verify_top_level_duplicate_metadata(root, dup, 0);
    fuzz_assert(cJSON_IsObject(dup));
    fuzz_assert(cJSON_GetArraySize(dup) == 0);
    fuzz_assert(!cJSON_Compare(root, dup, 1));
    fuzz_assert(!cJSON_Compare(root, dup, 0));
    expect_printed_text(dup, "{}");
    verify_roundtrip_if_safe(dup);

    cJSON_Delete(dup);
    cJSON_Delete(root);
}

static void run_string_scalar_scenario(const uint8_t **data, size_t *size) {
    cJSON *root = NULL;
    cJSON *dup = NULL;
    char *payload = NULL;

    payload = read_bounded_string(data, size, 32);
    if (payload == NULL) {
        payload = (char *)malloc(sizeof("scalar"));
        if (payload != NULL) {
            memcpy(payload, "scalar", sizeof("scalar"));
        }
    }
    if (payload == NULL) {
        return;
    }

    root = cJSON_CreateString(payload);
    if (root == NULL) {
        free(payload);
        return;
    }

    dup = cJSON_Duplicate(root, 0);
    if (dup != NULL) {
        verify_top_level_duplicate_metadata(root, dup, 0);
        fuzz_assert(cJSON_Compare(root, dup, 1));
        fuzz_assert(cJSON_Compare(root, dup, 0));
        fuzz_assert(cJSON_SetValuestring(dup, (strcmp(payload, "changed") == 0) ? "changed!" : "changed") != NULL);
        fuzz_assert(strcmp(root->valuestring, payload) == 0);
        fuzz_assert(!cJSON_Compare(root, dup, 1));
        verify_roundtrip_if_safe(dup);
    }

    cJSON_Delete(dup);
    cJSON_Delete(root);
    free(payload);
}

static void run_string_reference_scenario(const uint8_t **data, size_t *size) {
    cJSON *root = NULL;
    cJSON *dup = NULL;
    char *payload = NULL;

    payload = read_bounded_string(data, size, 32);
    if (payload == NULL) {
        payload = (char *)malloc(sizeof("shared"));
        if (payload != NULL) {
            memcpy(payload, "shared", sizeof("shared"));
        }
    }
    if (payload == NULL) {
        return;
    }

    root = cJSON_CreateStringReference(payload);
    if (root == NULL) {
        free(payload);
        return;
    }

    fuzz_assert((root->type & cJSON_IsReference) != 0);
    dup = cJSON_Duplicate(root, 0);
    if (dup != NULL) {
        verify_top_level_duplicate_metadata(root, dup, 0);
        fuzz_assert((dup->type & cJSON_IsReference) == 0);
        fuzz_assert(cJSON_Compare(root, dup, 1));
        fuzz_assert(cJSON_Compare(root, dup, 0));
        fuzz_assert(cJSON_SetValuestring(dup, (strcmp(payload, "changed") == 0) ? "changed!" : "changed") != NULL);
        fuzz_assert(strcmp(root->valuestring, payload) == 0);
        fuzz_assert(!cJSON_Compare(root, dup, 1));
        verify_roundtrip_if_safe(dup);
    }

    cJSON_Delete(dup);
    cJSON_Delete(root);
    free(payload);
}

static void run_const_key_scenario(void) {
    cJSON *root = NULL;
    cJSON *dup = NULL;
    cJSON *root_item = NULL;
    cJSON *dup_item = NULL;

    root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }

    root_item = cJSON_CreateNumber(5.0);
    if ((root_item == NULL) || !cJSON_AddItemToObjectCS(root, "CONSTKEY", root_item)) {
        cJSON_Delete(root_item);
        cJSON_Delete(root);
        return;
    }

    dup = cJSON_Duplicate(root, 1);
    if (dup == NULL) {
        cJSON_Delete(root);
        return;
    }

    verify_top_level_duplicate_metadata(root, dup, 1);
    verify_tree_distinct(root, dup);
    fuzz_assert(cJSON_Compare(root, dup, 1));

    root_item = cJSON_GetObjectItemCaseSensitive(root, "CONSTKEY");
    dup_item = cJSON_GetObjectItemCaseSensitive(dup, "CONSTKEY");
    fuzz_assert(root_item != NULL);
    fuzz_assert(dup_item != NULL);
    fuzz_assert((root_item->type & cJSON_StringIsConst) != 0);
    fuzz_assert((dup_item->type & cJSON_StringIsConst) != 0);
    fuzz_assert(root_item->string == dup_item->string);
    fuzz_assert(root_item != dup_item);

    cJSON_SetNumberValue(dup_item, 11.0);
    fuzz_assert(root_item->valueint == 5);
    fuzz_assert(!cJSON_Compare(root, dup, 1));

    cJSON_Delete(dup);
    cJSON_Delete(root);
}

static void run_circular_failure_scenario(void) {
    cJSON *root = NULL;
    cJSON *a = NULL;
    cJSON *b = NULL;
    cJSON *dup = NULL;

    root = cJSON_CreateArray();
    a = cJSON_CreateArray();
    b = cJSON_CreateArray();
    if ((root == NULL) || (a == NULL) || (b == NULL)) {
        cJSON_Delete(root);
        cJSON_Delete(a);
        cJSON_Delete(b);
        return;
    }

    fuzz_assert(cJSON_AddItemToArray(root, a));
    fuzz_assert(cJSON_AddItemToArray(a, b));
    fuzz_assert(cJSON_AddItemToArray(b, root));

    fuzz_assert(cJSON_Duplicate(root, 1) == NULL);

    dup = cJSON_Duplicate(root, 0);
    if (dup != NULL) {
        verify_top_level_duplicate_metadata(root, dup, 0);
        fuzz_assert(cJSON_IsArray(dup));
        fuzz_assert(cJSON_GetArraySize(dup) == 0);
        expect_printed_text(dup, "[]");
        verify_roundtrip_if_safe(dup);
    }

    fuzz_assert(cJSON_DetachItemFromArray(b, 0) == root);
    cJSON_Delete(dup);
    cJSON_Delete(root);
}

static void run_parsed_duplicate_scenario(const uint8_t *data, size_t size) {
    cJSON *root = NULL;
    cJSON *dup_recursive = NULL;
    cJSON *dup_shallow = NULL;
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

    dup_shallow = cJSON_Duplicate(root, 0);
    if (dup_shallow != NULL) {
        verify_top_level_duplicate_metadata(root, dup_shallow, 0);
        verify_roundtrip_if_safe(dup_shallow);
        if (root->child != NULL) {
            fuzz_assert(dup_shallow->child == NULL);
            if (cJSON_IsObject(root)) {
                expect_printed_text(dup_shallow, "{}");
            } else if (cJSON_IsArray(root)) {
                expect_printed_text(dup_shallow, "[]");
            }
            if (!tree_has_ambiguous_object_keys(root)) {
                fuzz_assert(!cJSON_Compare(root, dup_shallow, 1));
                fuzz_assert(!cJSON_Compare(root, dup_shallow, 0));
            }
        } else if (!tree_has_ambiguous_object_keys(root)) {
            fuzz_assert(cJSON_Compare(root, dup_shallow, 1));
            fuzz_assert(cJSON_Compare(root, dup_shallow, 0));
        }
    }

    dup_recursive = cJSON_Duplicate(root, 1);
    if (dup_recursive != NULL) {
        verify_top_level_duplicate_metadata(root, dup_recursive, 1);
        verify_tree_distinct(root, dup_recursive);
        verify_roundtrip_if_safe(dup_recursive);

        if (!tree_has_ambiguous_object_keys(root)) {
            fuzz_assert(cJSON_Compare(root, dup_recursive, 1));
            fuzz_assert(cJSON_Compare(root, dup_recursive, 0));
        }

        if (mutate_first_number_or_string(root, dup_recursive) &&
            !tree_has_ambiguous_object_keys(root)) {
            fuzz_assert(!cJSON_Compare(root, dup_recursive, 1));
        }
    }

    cJSON_Delete(dup_recursive);
    cJSON_Delete(dup_shallow);
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

    verify_null_duplicate_behavior();

    if (!read_u8(&data, &size, &raw_cases)) {
        return 0;
    }

    payload = data;
    payload_size = size;
    case_count = (size_t)(raw_cases % 20);

    for (size_t i = 0; (i < case_count) && (size > 0); ++i) {
        uint8_t opcode = 0;

        if (!read_u8(&data, &size, &opcode)) {
            break;
        }

        switch (opcode % 5) {
            case 0:
                run_recursive_seed_scenario(&data, &size);
                break;

            case 1:
                run_nonrecursive_seed_scenario();
                break;

            case 2:
                run_string_scalar_scenario(&data, &size);
                break;

            case 3:
                run_string_reference_scenario(&data, &size);
                break;

            case 4:
            default:
                if ((opcode & 1) == 0) {
                    run_const_key_scenario();
                } else {
                    run_circular_failure_scenario();
                }
                break;
        }
    }

    if (payload_size > 0) {
        run_parsed_duplicate_scenario(payload, payload_size);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
