#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ARRAY_COUNT 32
#define MAX_STRING_LEN 32

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

static int read_i32(const uint8_t **data, size_t *size, int *out) {
    union {
        uint32_t u32;
        int i32;
    } conv;

    conv.u32 = 0;

    if (*size >= sizeof(uint32_t)) {
        if (!read_bytes(data, size, (uint8_t *)&conv.u32, sizeof(uint32_t))) {
            return 0;
        }
    } else {
        size_t n = *size;

        if (n > 0) {
            memcpy(&conv.u32, *data, n);
            (*data) += n;
            (*size) -= n;
        }
    }

    *out = conv.i32;
    return 1;
}

static int read_float(const uint8_t **data, size_t *size, float *out) {
    union {
        uint32_t u32;
        float f;
    } conv;

    conv.u32 = 0;

    if (*size >= sizeof(uint32_t)) {
        if (!read_bytes(data, size, (uint8_t *)&conv.u32, sizeof(uint32_t))) {
            return 0;
        }
    } else {
        size_t n = *size;

        if (n > 0) {
            memcpy(&conv.u32, *data, n);
            (*data) += n;
            (*size) -= n;
        }
    }

    *out = conv.f;
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

static char *read_bounded_string(const uint8_t **data, size_t *size) {
    uint8_t raw_len = 0;
    size_t len = 0;
    char *out = NULL;

    if (!read_u8(data, size, &raw_len)) {
        return NULL;
    }

    len = (size_t)(raw_len % MAX_STRING_LEN);
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

static int number_to_valueint(double number) {
    fuzz_assert(!isnan(number));

    if (number >= INT_MAX) {
        return INT_MAX;
    }
    if (number <= (double)INT_MIN) {
        return INT_MIN;
    }
    return (int)number;
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

static void verify_roundtrip_if_safe(const cJSON *item) {
    char *printed = NULL;
    cJSON *parsed = NULL;

    if (!tree_is_roundtrip_safe(item)) {
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

static void verify_printed_text(const cJSON *item, const char *expected) {
    char *printed = NULL;

    fuzz_assert(item != NULL);
    fuzz_assert(expected != NULL);

    printed = cJSON_PrintUnformatted(item);
    fuzz_assert(printed != NULL);
    fuzz_assert(strcmp(printed, expected) == 0);
    free(printed);
}

static void verify_item_type(const cJSON *item, int base_type) {
    fuzz_assert(item != NULL);
    fuzz_assert((item->type & 0xFF) == base_type);
}

static void verify_fresh_item(const cJSON *item, int base_type) {
    verify_item_type(item, base_type);
    fuzz_assert(item->next == NULL);
    fuzz_assert(item->prev == NULL);
}

static void verify_number_item(const cJSON *item, double expected) {
    cJSON *reference = NULL;

    verify_item_type(item, cJSON_Number);

    if (!isfinite(expected)) {
        fuzz_assert(!isfinite(cJSON_GetNumberValue(item)));
        return;
    }

    reference = cJSON_CreateNumber(expected);
    if (reference != NULL) {
        fuzz_assert(cJSON_Compare(item, reference, 1));
        fuzz_assert(cJSON_Compare(item, reference, 0));
        cJSON_Delete(reference);
    }

    fuzz_assert(item->valueint == number_to_valueint(expected));
}

static void verify_global_invalid_behavior(void) {
    int dummy_ints[1] = {0};
    float dummy_floats[1] = {0.0f};
    double dummy_doubles[1] = {0.0};
    const char *dummy_strings[1] = {"x"};

    fuzz_assert(cJSON_CreateString(NULL) == NULL);
    fuzz_assert(cJSON_CreateRaw(NULL) == NULL);

    fuzz_assert(cJSON_CreateIntArray(NULL, 10) == NULL);
    fuzz_assert(cJSON_CreateFloatArray(NULL, 10) == NULL);
    fuzz_assert(cJSON_CreateDoubleArray(NULL, 10) == NULL);
    fuzz_assert(cJSON_CreateStringArray(NULL, 10) == NULL);

    fuzz_assert(cJSON_CreateIntArray(NULL, 0) == NULL);
    fuzz_assert(cJSON_CreateFloatArray(NULL, 0) == NULL);
    fuzz_assert(cJSON_CreateDoubleArray(NULL, 0) == NULL);
    fuzz_assert(cJSON_CreateStringArray(NULL, 0) == NULL);

    fuzz_assert(cJSON_CreateIntArray(dummy_ints, -1) == NULL);
    fuzz_assert(cJSON_CreateFloatArray(dummy_floats, -1) == NULL);
    fuzz_assert(cJSON_CreateDoubleArray(dummy_doubles, -1) == NULL);
    fuzz_assert(cJSON_CreateStringArray(dummy_strings, -1) == NULL);
}

static void run_primitive_scenario(const uint8_t **data, size_t *size) {
    uint8_t raw_bool = 0;
    double number = 0.0;
    cJSON *null_item = NULL;
    cJSON *true_item = NULL;
    cJSON *false_item = NULL;
    cJSON *bool_item = NULL;
    cJSON *number_item = NULL;
    cJSON *array_item = NULL;
    cJSON *object_item = NULL;

    (void)read_u8(data, size, &raw_bool);
    (void)read_double(data, size, &number);

    null_item = cJSON_CreateNull();
    true_item = cJSON_CreateTrue();
    false_item = cJSON_CreateFalse();
    bool_item = cJSON_CreateBool((raw_bool & 1) ? 1 : 0);
    number_item = cJSON_CreateNumber(number);
    array_item = cJSON_CreateArray();
    object_item = cJSON_CreateObject();

    if ((null_item == NULL) ||
        (true_item == NULL) ||
        (false_item == NULL) ||
        (bool_item == NULL) ||
        (number_item == NULL) ||
        (array_item == NULL) ||
        (object_item == NULL)) {
        cJSON_Delete(null_item);
        cJSON_Delete(true_item);
        cJSON_Delete(false_item);
        cJSON_Delete(bool_item);
        cJSON_Delete(number_item);
        cJSON_Delete(array_item);
        cJSON_Delete(object_item);
        return;
    }

    verify_fresh_item(null_item, cJSON_NULL);
    fuzz_assert(cJSON_IsNull(null_item));
    fuzz_assert(null_item->child == NULL);
    verify_printed_text(null_item, "null");
    verify_roundtrip_if_safe(null_item);

    verify_fresh_item(true_item, cJSON_True);
    fuzz_assert(cJSON_IsTrue(true_item));
    fuzz_assert(true_item->child == NULL);
    verify_printed_text(true_item, "true");
    verify_roundtrip_if_safe(true_item);

    verify_fresh_item(false_item, cJSON_False);
    fuzz_assert(cJSON_IsFalse(false_item));
    fuzz_assert(false_item->child == NULL);
    verify_printed_text(false_item, "false");
    verify_roundtrip_if_safe(false_item);

    verify_fresh_item(bool_item, (raw_bool & 1) ? cJSON_True : cJSON_False);
    fuzz_assert(cJSON_IsBool(bool_item));
    fuzz_assert(bool_item->child == NULL);
    verify_printed_text(bool_item, (raw_bool & 1) ? "true" : "false");
    verify_roundtrip_if_safe(bool_item);

    verify_number_item(number_item, number);
    fuzz_assert(number_item->child == NULL);
    if (isfinite(number)) {
        verify_roundtrip_if_safe(number_item);
    } else {
        verify_printed_text(number_item, "null");
    }

    verify_fresh_item(array_item, cJSON_Array);
    fuzz_assert(cJSON_IsArray(array_item));
    fuzz_assert(array_item->child == NULL);
    fuzz_assert(cJSON_GetArraySize(array_item) == 0);
    verify_printed_text(array_item, "[]");
    verify_roundtrip_if_safe(array_item);

    verify_fresh_item(object_item, cJSON_Object);
    fuzz_assert(cJSON_IsObject(object_item));
    fuzz_assert(object_item->child == NULL);
    fuzz_assert(cJSON_GetArraySize(object_item) == 0);
    verify_printed_text(object_item, "{}");
    verify_roundtrip_if_safe(object_item);

    cJSON_Delete(null_item);
    cJSON_Delete(true_item);
    cJSON_Delete(false_item);
    cJSON_Delete(bool_item);
    cJSON_Delete(number_item);
    cJSON_Delete(array_item);
    cJSON_Delete(object_item);
}

static void run_string_and_raw_scenario(const uint8_t **data, size_t *size) {
    static const char *raw_values[] = {
        "null",
        "true",
        "false",
        "0",
        "\"raw\"",
        "[]",
        "{}"
    };
    uint8_t raw_selector = 0;
    char *string_payload = NULL;
    char *raw_payload = NULL;
    cJSON *string_item = NULL;
    cJSON *raw_item = NULL;

    string_payload = read_bounded_string(data, size);
    if ((string_payload == NULL) || (string_payload[0] == '\0')) {
        free(string_payload);
        string_payload = (char *)malloc(sizeof("seed"));
        if (string_payload != NULL) {
            memcpy(string_payload, "seed", sizeof("seed"));
        }
    }
    if (string_payload == NULL) {
        return;
    }

    (void)read_u8(data, size, &raw_selector);
    raw_payload = (char *)malloc(strlen(raw_values[raw_selector % (sizeof(raw_values) / sizeof(raw_values[0]))]) + 1);
    if (raw_payload == NULL) {
        free(string_payload);
        return;
    }
    strcpy(raw_payload, raw_values[raw_selector % (sizeof(raw_values) / sizeof(raw_values[0]))]);

    string_item = cJSON_CreateString(string_payload);
    raw_item = cJSON_CreateRaw(raw_payload);
    if ((string_item == NULL) || (raw_item == NULL)) {
        cJSON_Delete(string_item);
        cJSON_Delete(raw_item);
        free(string_payload);
        free(raw_payload);
        return;
    }

    verify_fresh_item(string_item, cJSON_String);
    fuzz_assert(cJSON_IsString(string_item));
    fuzz_assert(string_item->child == NULL);
    fuzz_assert(cJSON_GetStringValue(string_item) != NULL);
    fuzz_assert(strcmp(cJSON_GetStringValue(string_item), string_payload) == 0);
    fuzz_assert(string_item->valuestring != string_payload);
    verify_roundtrip_if_safe(string_item);

    string_payload[0] = (string_payload[0] == 'Z') ? 'Y' : 'Z';
    fuzz_assert(strcmp(cJSON_GetStringValue(string_item), string_payload) != 0);

    verify_fresh_item(raw_item, cJSON_Raw);
    fuzz_assert(cJSON_IsRaw(raw_item));
    fuzz_assert(raw_item->child == NULL);
    fuzz_assert(raw_item->valuestring != NULL);
    fuzz_assert(strcmp(raw_item->valuestring, raw_payload) == 0);
    fuzz_assert(raw_item->valuestring != raw_payload);
    verify_printed_text(raw_item, raw_values[raw_selector % (sizeof(raw_values) / sizeof(raw_values[0]))]);

    raw_payload[0] = (raw_payload[0] == 'Z') ? 'Y' : 'Z';
    fuzz_assert(strcmp(raw_item->valuestring, raw_payload) != 0);

    cJSON_Delete(string_item);
    cJSON_Delete(raw_item);
    free(string_payload);
    free(raw_payload);
}

static void run_string_reference_scenario(const uint8_t **data, size_t *size) {
    char *payload = NULL;
    cJSON *string_ref = NULL;

    payload = read_bounded_string(data, size);
    if ((payload == NULL) || (payload[0] == '\0')) {
        free(payload);
        payload = (char *)malloc(sizeof("ref"));
        if (payload != NULL) {
            memcpy(payload, "ref", sizeof("ref"));
        }
    }
    if (payload == NULL) {
        return;
    }

    string_ref = cJSON_CreateStringReference(payload);
    if (string_ref == NULL) {
        free(payload);
        return;
    }

    verify_fresh_item(string_ref, cJSON_String);
    fuzz_assert((string_ref->type & cJSON_IsReference) != 0);
    fuzz_assert(string_ref->valuestring == payload);
    fuzz_assert(cJSON_IsString(string_ref));
    verify_roundtrip_if_safe(string_ref);

    payload[0] = (payload[0] == 'Q') ? 'R' : 'Q';
    fuzz_assert(string_ref->valuestring[0] == payload[0]);

    cJSON_Delete(string_ref);
    free(payload);
}

static void run_container_reference_scenario(const uint8_t **data, size_t *size) {
    int updated_value = 99;
    cJSON *number = NULL;
    cJSON *obj_ref = NULL;
    cJSON *arr_ref = NULL;
    cJSON *null_obj_ref = NULL;
    cJSON *null_arr_ref = NULL;

    (void)read_i32(data, size, &updated_value);

    number = cJSON_CreateNumber(42.0);
    if (number == NULL) {
        return;
    }

    obj_ref = cJSON_CreateObjectReference(number);
    arr_ref = cJSON_CreateArrayReference(number);
    null_obj_ref = cJSON_CreateObjectReference(NULL);
    null_arr_ref = cJSON_CreateArrayReference(NULL);
    if ((obj_ref == NULL) || (arr_ref == NULL) || (null_obj_ref == NULL) || (null_arr_ref == NULL)) {
        cJSON_Delete(number);
        cJSON_Delete(obj_ref);
        cJSON_Delete(arr_ref);
        cJSON_Delete(null_obj_ref);
        cJSON_Delete(null_arr_ref);
        return;
    }

    verify_fresh_item(obj_ref, cJSON_Object);
    fuzz_assert((obj_ref->type & cJSON_IsReference) != 0);
    fuzz_assert(obj_ref->child == number);

    verify_fresh_item(arr_ref, cJSON_Array);
    fuzz_assert((arr_ref->type & cJSON_IsReference) != 0);
    fuzz_assert(arr_ref->child == number);

    verify_fresh_item(null_obj_ref, cJSON_Object);
    fuzz_assert((null_obj_ref->type & cJSON_IsReference) != 0);
    fuzz_assert(null_obj_ref->child == NULL);

    verify_fresh_item(null_arr_ref, cJSON_Array);
    fuzz_assert((null_arr_ref->type & cJSON_IsReference) != 0);
    fuzz_assert(null_arr_ref->child == NULL);

    cJSON_SetNumberValue(number, updated_value);
    verify_number_item(obj_ref->child, (double)updated_value);
    verify_number_item(arr_ref->child, (double)updated_value);

    cJSON_Delete(number);
    cJSON_Delete(obj_ref);
    cJSON_Delete(arr_ref);
    cJSON_Delete(null_obj_ref);
    cJSON_Delete(null_arr_ref);
}

static void verify_int_array(cJSON *array, const int *values, int count) {
    fuzz_assert(array != NULL);
    verify_fresh_item(array, cJSON_Array);
    fuzz_assert(cJSON_IsArray(array));
    fuzz_assert(cJSON_GetArraySize(array) == count);

    for (int i = 0; i < count; ++i) {
        verify_number_item(cJSON_GetArrayItem(array, i), (double)values[i]);
    }

    verify_roundtrip_if_safe(array);
}

static void run_int_array_scenario(const uint8_t **data, size_t *size) {
    uint8_t raw_count = 0;
    int count = 0;
    int dummy = 0;
    int *values = NULL;
    cJSON *array = NULL;

    (void)read_u8(data, size, &raw_count);
    count = (int)(raw_count % MAX_ARRAY_COUNT);

    if (count > 0) {
        values = (int *)calloc((size_t)count, sizeof(int));
        if (values == NULL) {
            return;
        }
        for (int i = 0; i < count; ++i) {
            (void)read_i32(data, size, &values[i]);
        }
        array = cJSON_CreateIntArray(values, count);
    } else {
        array = cJSON_CreateIntArray(&dummy, 0);
    }

    if (array != NULL) {
        verify_int_array(array, (count > 0) ? values : &dummy, count);
        if (count > 0) {
            int before = cJSON_GetArrayItem(array, 0)->valueint;
            values[0] += 1;
            fuzz_assert(cJSON_GetArrayItem(array, 0)->valueint == before);
        } else {
            verify_printed_text(array, "[]");
        }
    }

    cJSON_Delete(array);
    free(values);
}

static void verify_float_array(cJSON *array, const float *values, int count) {
    fuzz_assert(array != NULL);
    verify_fresh_item(array, cJSON_Array);
    fuzz_assert(cJSON_GetArraySize(array) == count);

    for (int i = 0; i < count; ++i) {
        verify_number_item(cJSON_GetArrayItem(array, i), (double)values[i]);
    }

    verify_roundtrip_if_safe(array);
}

static void run_float_array_scenario(const uint8_t **data, size_t *size) {
    uint8_t raw_count = 0;
    int count = 0;
    float dummy = 0.0f;
    float *values = NULL;
    cJSON *array = NULL;

    (void)read_u8(data, size, &raw_count);
    count = (int)(raw_count % MAX_ARRAY_COUNT);

    if (count > 0) {
        values = (float *)calloc((size_t)count, sizeof(float));
        if (values == NULL) {
            return;
        }
        for (int i = 0; i < count; ++i) {
            (void)read_float(data, size, &values[i]);
        }
        array = cJSON_CreateFloatArray(values, count);
    } else {
        array = cJSON_CreateFloatArray(&dummy, 0);
    }

    if (array != NULL) {
        verify_float_array(array, (count > 0) ? values : &dummy, count);
        if (count > 0) {
            char *before = cJSON_PrintUnformatted(array);
            values[0] += 1.0f;
            char *after = NULL;
            fuzz_assert(before != NULL);
            after = cJSON_PrintUnformatted(array);
            fuzz_assert(after != NULL);
            fuzz_assert(strcmp(before, after) == 0);
            free(after);
            free(before);
        } else {
            verify_printed_text(array, "[]");
        }
    }

    cJSON_Delete(array);
    free(values);
}

static void verify_double_array(cJSON *array, const double *values, int count) {
    fuzz_assert(array != NULL);
    verify_fresh_item(array, cJSON_Array);
    fuzz_assert(cJSON_GetArraySize(array) == count);

    for (int i = 0; i < count; ++i) {
        verify_number_item(cJSON_GetArrayItem(array, i), values[i]);
    }

    verify_roundtrip_if_safe(array);
}

static void run_double_array_scenario(const uint8_t **data, size_t *size) {
    uint8_t raw_count = 0;
    int count = 0;
    double dummy = 0.0;
    double *values = NULL;
    cJSON *array = NULL;

    (void)read_u8(data, size, &raw_count);
    count = (int)(raw_count % MAX_ARRAY_COUNT);

    if (count > 0) {
        values = (double *)calloc((size_t)count, sizeof(double));
        if (values == NULL) {
            return;
        }
        for (int i = 0; i < count; ++i) {
            (void)read_double(data, size, &values[i]);
        }
        array = cJSON_CreateDoubleArray(values, count);
    } else {
        array = cJSON_CreateDoubleArray(&dummy, 0);
    }

    if (array != NULL) {
        verify_double_array(array, (count > 0) ? values : &dummy, count);
        if (count > 0) {
            char *before = cJSON_PrintUnformatted(array);
            values[0] += 1.0;
            char *after = NULL;
            fuzz_assert(before != NULL);
            after = cJSON_PrintUnformatted(array);
            fuzz_assert(after != NULL);
            fuzz_assert(strcmp(before, after) == 0);
            free(after);
            free(before);
        } else {
            verify_printed_text(array, "[]");
        }
    }

    cJSON_Delete(array);
    free(values);
}

static void run_string_array_scenario(const uint8_t **data, size_t *size) {
    uint8_t raw_count = 0;
    int count = 0;
    const char *dummy_strings[1] = {"unused"};
    char **values = NULL;
    cJSON *array = NULL;

    (void)read_u8(data, size, &raw_count);
    count = (int)(raw_count % MAX_ARRAY_COUNT);

    if (count > 0) {
        values = (char **)calloc((size_t)count, sizeof(char *));
        if (values == NULL) {
            return;
        }

        for (int i = 0; i < count; ++i) {
            values[i] = read_bounded_string(data, size);
            if ((values[i] == NULL) || (values[i][0] == '\0')) {
                free(values[i]);
                values[i] = (char *)malloc(sizeof("a"));
                if (values[i] == NULL) {
                    count = i;
                    break;
                }
                memcpy(values[i], "a", sizeof("a"));
            }
        }

        if (count > 0) {
            array = cJSON_CreateStringArray((const char * const *)values, count);
        }
    } else {
        array = cJSON_CreateStringArray(dummy_strings, 0);
    }

    if (array != NULL) {
        verify_fresh_item(array, cJSON_Array);
        fuzz_assert(cJSON_GetArraySize(array) == count);

        for (int i = 0; i < count; ++i) {
            cJSON *entry = cJSON_GetArrayItem(array, i);
            verify_item_type(entry, cJSON_String);
            fuzz_assert(cJSON_IsString(entry));
            fuzz_assert(entry->valuestring != NULL);
            fuzz_assert(strcmp(entry->valuestring, values[i]) == 0);
            fuzz_assert(entry->valuestring != values[i]);
        }

        verify_roundtrip_if_safe(array);
        if (count > 0) {
            char before = cJSON_GetArrayItem(array, 0)->valuestring[0];
            values[0][0] = (values[0][0] == 'Z') ? 'Y' : 'Z';
            fuzz_assert(cJSON_GetArrayItem(array, 0)->valuestring[0] == before);
        } else {
            verify_printed_text(array, "[]");
        }
    }

    cJSON_Delete(array);
    if (values != NULL) {
        for (int i = 0; i < count; ++i) {
            free(values[i]);
        }
    }
    free(values);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    uint8_t raw_cases = 0;
    size_t case_count = 0;

    if ((data == NULL) || (size == 0)) {
        return 0;
    }

    verify_global_invalid_behavior();

    if (!read_u8(&data, &size, &raw_cases)) {
        return 0;
    }

    case_count = (size_t)(raw_cases % 24);

    for (size_t i = 0; (i < case_count) && (size > 0); ++i) {
        uint8_t opcode = 0;

        if (!read_u8(&data, &size, &opcode)) {
            break;
        }

        switch (opcode % 8) {
            case 0:
                run_primitive_scenario(&data, &size);
                break;

            case 1:
                run_string_and_raw_scenario(&data, &size);
                break;

            case 2:
                run_string_reference_scenario(&data, &size);
                break;

            case 3:
                run_container_reference_scenario(&data, &size);
                break;

            case 4:
                run_int_array_scenario(&data, &size);
                break;

            case 5:
                run_float_array_scenario(&data, &size);
                break;

            case 6:
                run_double_array_scenario(&data, &size);
                break;

            case 7:
            default:
                run_string_array_scenario(&data, &size);
                break;
        }
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
