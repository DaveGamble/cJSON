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

typedef struct {
    cJSON *left;
    cJSON *right;
} compare_pair;

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

static int read_small_int(const uint8_t **data, size_t *size, int *out) {
    uint8_t raw = 0;

    if (!read_u8(data, size, &raw)) {
        return 0;
    }

    *out = (int)(raw % 201) - 100;
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

static size_t find_split(const uint8_t *data, size_t size) {
    for (size_t i = 1; i + 1 < size; ++i) {
        if (data[i] == '|' || data[i] == '\n') {
            return i;
        }
    }

    return size / 2;
}

static compare_pair make_empty_pair(void) {
    compare_pair pair;
    pair.left = NULL;
    pair.right = NULL;
    return pair;
}

static void delete_pair(compare_pair *pair) {
    if (pair == NULL) {
        return;
    }

    cJSON_Delete(pair->left);
    cJSON_Delete(pair->right);
    pair->left = NULL;
    pair->right = NULL;
}

static const char *select_raw_literal(uint8_t selector) {
    static const char *literals[] = {
        "null",
        "true",
        "false",
        "0",
        "42",
        "\"raw\"",
        "[]",
        "{}"
    };

    return literals[selector % (sizeof(literals) / sizeof(literals[0]))];
}

static cJSON *create_seed_tree(void) {
    cJSON *root = NULL;
    cJSON *nested = NULL;
    cJSON *meta = NULL;
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

    meta = cJSON_AddObjectToObject(root, "meta");
    if (meta == NULL) {
        goto fail;
    }
    if (cJSON_AddStringToObject(meta, "kind", "seed") == NULL) {
        goto fail;
    }
    if (cJSON_AddFalseToObject(meta, "ready") == NULL) {
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

static compare_pair create_seed_pair(void) {
    compare_pair pair = make_empty_pair();

    pair.left = create_seed_tree();
    if (pair.left == NULL) {
        return pair;
    }

    pair.right = cJSON_Duplicate(pair.left, 1);
    if (pair.right == NULL) {
        delete_pair(&pair);
    }

    return pair;
}

static compare_pair create_case_variant_pair(void) {
    compare_pair pair = make_empty_pair();

    pair.left = cJSON_CreateObject();
    pair.right = cJSON_CreateObject();
    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return make_empty_pair();
    }

    if ((cJSON_AddFalseToObject(pair.left, "False") == NULL) ||
        (cJSON_AddTrueToObject(pair.left, "true") == NULL) ||
        (cJSON_AddNumberToObject(pair.left, "number", 42.0) == NULL) ||
        (cJSON_AddFalseToObject(pair.right, "false") == NULL) ||
        (cJSON_AddTrueToObject(pair.right, "true") == NULL) ||
        (cJSON_AddNumberToObject(pair.right, "number", 42.0) == NULL)) {
        delete_pair(&pair);
        return make_empty_pair();
    }

    return pair;
}

static compare_pair create_object_order_pair(void) {
    compare_pair pair = make_empty_pair();
    cJSON *left_nested = NULL;
    cJSON *right_nested = NULL;

    pair.left = cJSON_CreateObject();
    pair.right = cJSON_CreateObject();
    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return make_empty_pair();
    }

    if ((cJSON_AddNumberToObject(pair.left, "a", 1.0) == NULL) ||
        (cJSON_AddStringToObject(pair.left, "b", "x") == NULL)) {
        delete_pair(&pair);
        return make_empty_pair();
    }
    left_nested = cJSON_AddObjectToObject(pair.left, "nested");
    if ((left_nested == NULL) || (cJSON_AddTrueToObject(left_nested, "ok") == NULL)) {
        delete_pair(&pair);
        return make_empty_pair();
    }

    right_nested = cJSON_AddObjectToObject(pair.right, "nested");
    if ((right_nested == NULL) ||
        (cJSON_AddTrueToObject(right_nested, "ok") == NULL) ||
        (cJSON_AddStringToObject(pair.right, "b", "x") == NULL) ||
        (cJSON_AddNumberToObject(pair.right, "a", 1.0) == NULL)) {
        delete_pair(&pair);
        return make_empty_pair();
    }

    return pair;
}

static compare_pair create_array_order_pair(void) {
    compare_pair pair = make_empty_pair();

    pair.left = cJSON_CreateArray();
    pair.right = cJSON_CreateArray();
    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return make_empty_pair();
    }

    if (!cJSON_AddItemToArray(pair.left, cJSON_CreateNumber(1.0)) ||
        !cJSON_AddItemToArray(pair.left, cJSON_CreateNumber(2.0)) ||
        !cJSON_AddItemToArray(pair.left, cJSON_CreateString("x")) ||
        !cJSON_AddItemToArray(pair.right, cJSON_CreateNumber(2.0)) ||
        !cJSON_AddItemToArray(pair.right, cJSON_CreateNumber(1.0)) ||
        !cJSON_AddItemToArray(pair.right, cJSON_CreateString("x"))) {
        delete_pair(&pair);
        return make_empty_pair();
    }

    return pair;
}

static compare_pair create_raw_pair(uint8_t left_selector, uint8_t right_selector) {
    compare_pair pair = make_empty_pair();

    pair.left = cJSON_CreateRaw(select_raw_literal(left_selector));
    pair.right = cJSON_CreateRaw(select_raw_literal(right_selector));
    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
    }

    return pair;
}

static cJSON *get_nested_value(cJSON *root) {
    cJSON *nested = NULL;

    if (root == NULL) {
        return NULL;
    }

    nested = cJSON_GetObjectItemCaseSensitive(root, "nested");
    if (nested == NULL) {
        return NULL;
    }

    return cJSON_GetObjectItemCaseSensitive(nested, "value");
}

static cJSON *get_nested_label(cJSON *root) {
    cJSON *nested = NULL;

    if (root == NULL) {
        return NULL;
    }

    nested = cJSON_GetObjectItemCaseSensitive(root, "nested");
    if (nested == NULL) {
        return NULL;
    }

    return cJSON_GetObjectItemCaseSensitive(nested, "label");
}

static void set_nested_number(cJSON *root, int value) {
    cJSON *number = get_nested_value(root);

    fuzz_assert(number != NULL);
    fuzz_assert(cJSON_IsNumber(number));
    cJSON_SetNumberValue(number, value);
}

static void set_nested_label(cJSON *root, const char *value) {
    cJSON *label = get_nested_label(root);

    fuzz_assert(label != NULL);
    fuzz_assert(cJSON_IsString(label));
    fuzz_assert(cJSON_SetValuestring(label, value) != NULL);
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

static void verify_reflexive(const cJSON *item) {
    fuzz_assert(item != NULL);
    fuzz_assert(cJSON_Compare(item, item, 1));
    fuzz_assert(cJSON_Compare(item, item, 0));
}

static void verify_null_and_invalid_behavior(const cJSON *item) {
    cJSON invalid[1];

    memset(invalid, 0, sizeof(invalid));

    fuzz_assert(!cJSON_Compare(NULL, NULL, 1));
    fuzz_assert(!cJSON_Compare(NULL, NULL, 0));
    fuzz_assert(!cJSON_Compare(invalid, invalid, 1));
    fuzz_assert(!cJSON_Compare(invalid, invalid, 0));

    invalid->type = cJSON_Number | cJSON_String;
    fuzz_assert(!cJSON_Compare(invalid, invalid, 1));
    fuzz_assert(!cJSON_Compare(invalid, invalid, 0));

    if (item != NULL) {
        fuzz_assert(!cJSON_Compare(NULL, item, 1));
        fuzz_assert(!cJSON_Compare(item, NULL, 1));
        fuzz_assert(!cJSON_Compare(NULL, item, 0));
        fuzz_assert(!cJSON_Compare(item, NULL, 0));
    }
}

static void verify_roundtrip_if_safe(const cJSON *item) {
    char *printed = NULL;
    cJSON *parsed = NULL;

    if ((item == NULL) ||
        tree_contains_raw(item) ||
        tree_contains_nonfinite_number(item) ||
        tree_has_ambiguous_object_keys(item)) {
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

static void verify_duplicate_equality(const cJSON *item) {
    cJSON *dup = NULL;

    if ((item == NULL) ||
        tree_contains_nonfinite_number(item) ||
        tree_has_ambiguous_object_keys(item)) {
        return;
    }

    dup = cJSON_Duplicate(item, 1);
    if (dup == NULL) {
        return;
    }

    fuzz_assert(cJSON_Compare(item, dup, 1));
    fuzz_assert(cJSON_Compare(dup, item, 1));
    fuzz_assert(cJSON_Compare(item, dup, 0));
    fuzz_assert(cJSON_Compare(dup, item, 0));

    cJSON_Delete(dup);
}

static void verify_pair_relation(const cJSON *left,
                                 const cJSON *right,
                                 cJSON_bool expect_strict,
                                 cJSON_bool expect_loose) {
    cJSON *left_dup = NULL;
    cJSON *right_dup = NULL;
    cJSON_bool strict_lr = 0;
    cJSON_bool strict_rl = 0;
    cJSON_bool loose_lr = 0;
    cJSON_bool loose_rl = 0;

    fuzz_assert(left != NULL);
    fuzz_assert(right != NULL);

    strict_lr = cJSON_Compare(left, right, 1);
    strict_rl = cJSON_Compare(right, left, 1);
    loose_lr = cJSON_Compare(left, right, 0);
    loose_rl = cJSON_Compare(right, left, 0);

    fuzz_assert(strict_lr == strict_rl);
    fuzz_assert(loose_lr == loose_rl);
    fuzz_assert(strict_lr == expect_strict);
    fuzz_assert(loose_lr == expect_loose);
    fuzz_assert(!expect_strict || expect_loose);

    verify_reflexive(left);
    verify_reflexive(right);
    verify_duplicate_equality(left);
    verify_duplicate_equality(right);
    verify_roundtrip_if_safe(left);
    verify_roundtrip_if_safe(right);
    verify_null_and_invalid_behavior(left);
    verify_null_and_invalid_behavior(right);

    left_dup = cJSON_Duplicate(left, 1);
    right_dup = cJSON_Duplicate(right, 1);
    if ((left_dup != NULL) &&
        (right_dup != NULL) &&
        !tree_contains_nonfinite_number(left) &&
        !tree_contains_nonfinite_number(right) &&
        !tree_has_ambiguous_object_keys(left) &&
        !tree_has_ambiguous_object_keys(right)) {
        fuzz_assert(cJSON_Compare(left_dup, right_dup, 1) == expect_strict);
        fuzz_assert(cJSON_Compare(right_dup, left_dup, 1) == expect_strict);
        fuzz_assert(cJSON_Compare(left_dup, right_dup, 0) == expect_loose);
        fuzz_assert(cJSON_Compare(right_dup, left_dup, 0) == expect_loose);
    }

    cJSON_Delete(left_dup);
    cJSON_Delete(right_dup);
}

static void run_seed_equality_scenario(void) {
    compare_pair pair = create_seed_pair();

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    verify_pair_relation(pair.left, pair.right, 1, 1);
    delete_pair(&pair);
}

static void run_same_number_scenario(const uint8_t **data, size_t *size) {
    compare_pair pair = create_seed_pair();
    int value = 0;

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    (void)read_small_int(data, size, &value);
    set_nested_number(pair.left, value);
    set_nested_number(pair.right, value);

    verify_pair_relation(pair.left, pair.right, 1, 1);
    delete_pair(&pair);
}

static void run_different_number_scenario(const uint8_t **data, size_t *size) {
    compare_pair pair = create_seed_pair();
    int left_value = 0;
    int right_value = 0;

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    (void)read_small_int(data, size, &left_value);
    (void)read_small_int(data, size, &right_value);
    if (left_value == right_value) {
        right_value += 1;
    }

    set_nested_number(pair.left, left_value);
    set_nested_number(pair.right, right_value);

    verify_pair_relation(pair.left, pair.right, 0, 0);
    delete_pair(&pair);
}

static void run_same_string_scenario(const uint8_t **data, size_t *size) {
    compare_pair pair = create_seed_pair();
    char *payload = NULL;

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    payload = read_bounded_string(data, size, 24);
    if (payload == NULL) {
        payload = (char *)malloc(sizeof("same"));
        if (payload == NULL) {
            delete_pair(&pair);
            return;
        }
        memcpy(payload, "same", sizeof("same"));
    }

    set_nested_label(pair.left, payload);
    set_nested_label(pair.right, payload);

    verify_pair_relation(pair.left, pair.right, 1, 1);

    free(payload);
    delete_pair(&pair);
}

static void run_different_string_scenario(const uint8_t **data, size_t *size) {
    compare_pair pair = create_seed_pair();
    char *left_payload = NULL;
    char *right_payload = NULL;

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    left_payload = read_bounded_string(data, size, 24);
    right_payload = read_bounded_string(data, size, 24);
    if (left_payload == NULL) {
        left_payload = (char *)malloc(sizeof("left"));
        if (left_payload == NULL) {
            delete_pair(&pair);
            return;
        }
        memcpy(left_payload, "left", sizeof("left"));
    }
    if (right_payload == NULL) {
        right_payload = (char *)malloc(sizeof("right"));
        if (right_payload == NULL) {
            free(left_payload);
            delete_pair(&pair);
            return;
        }
        memcpy(right_payload, "right", sizeof("right"));
    }
    if (strcmp(left_payload, right_payload) == 0) {
        size_t len = strlen(right_payload);
        char *replacement = (char *)malloc(len + 2);

        if (replacement == NULL) {
            free(left_payload);
            free(right_payload);
            delete_pair(&pair);
            return;
        }

        memcpy(replacement, right_payload, len);
        replacement[len] = '!';
        replacement[len + 1] = '\0';
        free(right_payload);
        right_payload = replacement;
    }

    set_nested_label(pair.left, left_payload);
    set_nested_label(pair.right, right_payload);

    verify_pair_relation(pair.left, pair.right, 0, 0);

    free(left_payload);
    free(right_payload);
    delete_pair(&pair);
}

static void run_case_sensitivity_scenario(void) {
    compare_pair pair = create_case_variant_pair();

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    verify_pair_relation(pair.left, pair.right, 0, 1);
    delete_pair(&pair);
}

static void run_object_order_scenario(void) {
    compare_pair pair = create_object_order_pair();

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    verify_pair_relation(pair.left, pair.right, 1, 1);
    delete_pair(&pair);
}

static void run_array_order_scenario(void) {
    compare_pair pair = create_array_order_pair();

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    verify_pair_relation(pair.left, pair.right, 0, 0);
    delete_pair(&pair);
}

static void run_subset_scenario(void) {
    compare_pair pair = create_seed_pair();

    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    cJSON_DeleteItemFromObjectCaseSensitive(pair.right, "meta");
    verify_pair_relation(pair.left, pair.right, 0, 0);
    delete_pair(&pair);
}

static void run_raw_scenario(const uint8_t **data, size_t *size) {
    compare_pair pair;
    uint8_t left_selector = 0;
    uint8_t right_selector = 0;
    cJSON_bool expect_equal = 0;

    (void)read_u8(data, size, &left_selector);
    (void)read_u8(data, size, &right_selector);

    pair = create_raw_pair(left_selector, right_selector);
    if ((pair.left == NULL) || (pair.right == NULL)) {
        delete_pair(&pair);
        return;
    }

    expect_equal = (strcmp(select_raw_literal(left_selector),
                           select_raw_literal(right_selector)) == 0);
    verify_pair_relation(pair.left, pair.right, expect_equal, expect_equal);
    delete_pair(&pair);
}

static void run_parsed_pair_scenario(const uint8_t *data, size_t size) {
    char *left_text = NULL;
    char *right_text = NULL;
    cJSON *left = NULL;
    cJSON *right = NULL;
    size_t split = 0;
    size_t left_len = 0;
    size_t right_len = 0;

    if ((data == NULL) || (size < 2)) {
        return;
    }

    split = find_split(data, size);
    if ((split == 0) || (split >= (size - 1))) {
        return;
    }

    left_len = split;
    right_len = size - split - 1;

    left_text = (char *)malloc(left_len + 1);
    right_text = (char *)malloc(right_len + 1);
    if ((left_text == NULL) || (right_text == NULL)) {
        free(left_text);
        free(right_text);
        return;
    }

    memcpy(left_text, data, left_len);
    left_text[left_len] = '\0';
    memcpy(right_text, data + split + 1, right_len);
    right_text[right_len] = '\0';

    left = cJSON_Parse(left_text);
    right = cJSON_Parse(right_text);

    if (left != NULL) {
        verify_reflexive(left);
        verify_duplicate_equality(left);
        verify_roundtrip_if_safe(left);
        verify_null_and_invalid_behavior(left);
    }

    if (right != NULL) {
        verify_reflexive(right);
        verify_duplicate_equality(right);
        verify_roundtrip_if_safe(right);
        verify_null_and_invalid_behavior(right);
    }

    if ((left != NULL) && (right != NULL)) {
        verify_pair_relation(left,
                             right,
                             cJSON_Compare(left, right, 1),
                             cJSON_Compare(left, right, 0));
    }

    cJSON_Delete(left);
    cJSON_Delete(right);
    free(left_text);
    free(right_text);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    const uint8_t *payload_data = NULL;
    size_t payload_size = 0;
    uint8_t raw_cases = 0;
    size_t case_count = 0;

    if ((data == NULL) || (size == 0)) {
        return 0;
    }

    verify_null_and_invalid_behavior(NULL);

    if (!read_u8(&data, &size, &raw_cases)) {
        return 0;
    }

    payload_data = data;
    payload_size = size;
    case_count = (size_t)(raw_cases % 24);

    for (size_t i = 0; (i < case_count) && (size > 0); ++i) {
        uint8_t opcode = 0;

        if (!read_u8(&data, &size, &opcode)) {
            break;
        }

        switch (opcode % 9) {
            case 0:
                run_seed_equality_scenario();
                break;

            case 1:
                run_same_number_scenario(&data, &size);
                break;

            case 2:
                run_different_number_scenario(&data, &size);
                break;

            case 3:
                run_same_string_scenario(&data, &size);
                break;

            case 4:
                run_different_string_scenario(&data, &size);
                break;

            case 5:
                run_case_sensitivity_scenario();
                break;

            case 6:
                run_object_order_scenario();
                break;

            case 7:
                run_array_order_scenario();
                break;

            case 8:
            default:
                if ((opcode & 1) == 0) {
                    run_subset_scenario();
                } else {
                    run_raw_scenario(&data, &size);
                }
                break;
        }
    }

    if (payload_size > 0) {
        run_parsed_pair_scenario(payload_data, payload_size);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
