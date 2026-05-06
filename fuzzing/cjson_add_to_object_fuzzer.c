#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

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

static char *dup_cstr(const char *s) {
    size_t n = strlen(s);
    char *copy = (char *)malloc(n + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, s, n + 1);
    return copy;
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

static char *choose_known_key(uint8_t selector) {
    static const char *keys[] = {
        "alpha",
        "Bravo",
        "CHARLIE",
        "delta",
        "Echo",
        "foxtrot",
        "nestedObj",
        "childObj",
        "items"
    };

    return dup_cstr(keys[selector % (sizeof(keys) / sizeof(keys[0]))]);
}

static char *choose_add_key(const uint8_t **data, size_t *size) {
    uint8_t mode = 0;
    uint8_t sel = 0;

    if (!read_u8(data, size, &mode)) {
        return NULL;
    }

    if ((mode & 3) == 0) {
        if (!read_u8(data, size, &sel)) {
            return NULL;
        }
        return choose_known_key(sel);
    }

    return read_bounded_string(data, size, 24);
}

static void seed_object(cJSON *root, cJSON **containers, size_t *container_count) {
    cJSON *nested = NULL;
    cJSON *child = NULL;
    cJSON *arr = NULL;

    if (root == NULL) {
        return;
    }

    cJSON_AddNumberToObject(root, "alpha", 1.0);
    cJSON_AddStringToObject(root, "Bravo", "seed");
    cJSON_AddBoolToObject(root, "CHARLIE", 1);
    cJSON_AddNullToObject(root, "delta");

    nested = cJSON_AddObjectToObject(root, "nestedObj");
    if (nested != NULL) {
        cJSON_AddNumberToObject(nested, "inner", 7.0);
    }

    child = cJSON_AddObjectToObject(root, "childObj");
    if (child != NULL) {
        cJSON_AddStringToObject(child, "name", "x");
    }

    arr = cJSON_AddArrayToObject(root, "items");
    if (arr != NULL) {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber(1.0));
        cJSON_AddItemToArray(arr, cJSON_CreateString("a"));
    }

    if (containers != NULL && container_count != NULL) {
        *container_count = 0;
        containers[(*container_count)++] = root;
        if (nested != NULL && *container_count < 8) {
            containers[(*container_count)++] = nested;
        }
        if (child != NULL && *container_count < 8) {
            containers[(*container_count)++] = child;
        }
    }
}

static void clear_root_object(cJSON *root) {
    while (root != NULL && root->child != NULL) {
        cJSON *detached = cJSON_DetachItemViaPointer(root, root->child);
        if (detached == NULL) {
            break;
        }
        cJSON_Delete(detached);
    }
}

static void rebuild_object(cJSON *root, cJSON **containers, size_t *container_count) {
    if (root == NULL) {
        return;
    }

    clear_root_object(root);
    seed_object(root, containers, container_count);
}

static void maybe_rebuild(const uint8_t **data, size_t *size, cJSON *root,
                          cJSON **containers, size_t *container_count) {
    uint8_t control = 0;
    if (!read_u8(data, size, &control)) {
        return;
    }

    if ((control & 7) == 0) {
        rebuild_object(root, containers, container_count);
    }
}

static cJSON *create_leaf_item(const uint8_t **data, size_t *size, uint8_t kind) {
    char *payload = NULL;
    double number = 0.0;
    uint8_t b = 0;

    switch (kind % 6) {
        case 0:
            (void)read_double(data, size, &number);
            return cJSON_CreateNumber(number);

        case 1:
            payload = read_bounded_string(data, size, 48);
            if (payload == NULL) {
                return cJSON_CreateString("");
            }
            {
                cJSON *item = cJSON_CreateString(payload);
                free(payload);
                return item;
            }

        case 2:
            payload = read_bounded_string(data, size, 48);
            if (payload == NULL) {
                return cJSON_CreateRaw("null");
            }
            {
                cJSON *item = cJSON_CreateRaw(payload);
                free(payload);
                return item;
            }

        case 3:
            (void)read_u8(data, size, &b);
            return cJSON_CreateBool((b & 1) ? 1 : 0);

        case 4:
            return cJSON_CreateTrue();

        case 5:
        default:
            return cJSON_CreateNull();
    }
}

static cJSON *create_add_item(const uint8_t **data, size_t *size, int depth) {
    uint8_t kind = 0;

    if (!read_u8(data, size, &kind)) {
        return cJSON_CreateNull();
    }

    if (depth <= 0) {
        return create_leaf_item(data, size, kind);
    }

    switch (kind % 8) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            return create_leaf_item(data, size, kind);

        case 6: {
            cJSON *obj = cJSON_CreateObject();
            uint8_t count = 0;
            if (obj == NULL) {
                return NULL;
            }

            (void)read_u8(data, size, &count);
            for (size_t i = 0; i < (size_t)(count % 3); ++i) {
                char *k = read_bounded_string(data, size, 16);
                cJSON *v = NULL;

                if (k == NULL) {
                    break;
                }

                v = create_add_item(data, size, depth - 1);
                if (v == NULL || !cJSON_AddItemToObject(obj, k, v)) {
                    cJSON_Delete(v);
                    free(k);
                    cJSON_Delete(obj);
                    return NULL;
                }
                free(k);
            }
            return obj;
        }

        case 7:
        default: {
            cJSON *arr = cJSON_CreateArray();
            uint8_t count = 0;
            if (arr == NULL) {
                return NULL;
            }

            (void)read_u8(data, size, &count);
            for (size_t i = 0; i < (size_t)(count % 4); ++i) {
                cJSON *elem = create_add_item(data, size, depth - 1);
                if (elem == NULL || !cJSON_AddItemToArray(arr, elem)) {
                    cJSON_Delete(elem);
                    cJSON_Delete(arr);
                    return NULL;
                }
            }
            return arr;
        }
    }
}

static void assert_add_postconditions(cJSON *target, const char *key, int valid_inputs,
                                      int add_ok, int size_before, int size_after) {
    if (!valid_inputs) {
        if (add_ok || size_after != size_before) {
            abort();
        }
        return;
    }

    if (!add_ok || size_after != (size_before + 1)) {
        abort();
    }

    if (cJSON_GetObjectItemCaseSensitive(target, key) == NULL) {
        abort();
    }
}

static void op_add_number(const uint8_t **data, size_t *size, cJSON *target, const char *key,
                          int valid_inputs) {
    double number = 0.0;
    cJSON *added = NULL;
    int size_before = 0;
    int size_after = 0;

    (void)read_double(data, size, &number);
    size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    added = cJSON_AddNumberToObject(target, key, number);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;

    assert_add_postconditions(target, key, valid_inputs, added != NULL, size_before, size_after);
}

static void op_add_string(const uint8_t **data, size_t *size, cJSON *target, const char *key,
                          int valid_inputs) {
    char *payload = read_bounded_string(data, size, 48);
    cJSON *added = NULL;
    int size_before = 0;
    int size_after = 0;

    if (payload == NULL) {
        return;
    }

    size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    added = cJSON_AddStringToObject(target, key, payload);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;

    assert_add_postconditions(target, key, valid_inputs, added != NULL, size_before, size_after);
    free(payload);
}

static void op_add_raw(const uint8_t **data, size_t *size, cJSON *target, const char *key,
                       int valid_inputs) {
    char *payload = read_bounded_string(data, size, 48);
    cJSON *added = NULL;
    int size_before = 0;
    int size_after = 0;

    if (payload == NULL) {
        return;
    }

    size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    added = cJSON_AddRawToObject(target, key, payload);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;

    assert_add_postconditions(target, key, valid_inputs, added != NULL, size_before, size_after);
    free(payload);
}

static void op_add_true(cJSON *target, const char *key, int valid_inputs) {
    cJSON *added = NULL;
    int size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    int size_after = 0;

    added = cJSON_AddTrueToObject(target, key);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    assert_add_postconditions(target, key, valid_inputs, added != NULL, size_before, size_after);
}

static void op_add_false(cJSON *target, const char *key, int valid_inputs) {
    cJSON *added = NULL;
    int size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    int size_after = 0;

    added = cJSON_AddFalseToObject(target, key);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    assert_add_postconditions(target, key, valid_inputs, added != NULL, size_before, size_after);
}

static void op_add_null(cJSON *target, const char *key, int valid_inputs) {
    cJSON *added = NULL;
    int size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    int size_after = 0;

    added = cJSON_AddNullToObject(target, key);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    assert_add_postconditions(target, key, valid_inputs, added != NULL, size_before, size_after);
}

static void op_add_bool(const uint8_t **data, size_t *size, cJSON *target, const char *key,
                        int valid_inputs) {
    uint8_t b = 0;
    cJSON *added = NULL;
    int size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    int size_after = 0;

    (void)read_u8(data, size, &b);
    added = cJSON_AddBoolToObject(target, key, (b & 1) ? 1 : 0);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    assert_add_postconditions(target, key, valid_inputs, added != NULL, size_before, size_after);
}

static void op_add_object(cJSON *target, const char *key, int valid_inputs,
                          cJSON **containers, size_t *container_count) {
    cJSON *obj = NULL;
    int size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    int size_after = 0;

    obj = cJSON_AddObjectToObject(target, key);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    assert_add_postconditions(target, key, valid_inputs, obj != NULL, size_before, size_after);

    if (obj != NULL && containers != NULL && container_count != NULL && *container_count < 8) {
        containers[(*container_count)++] = obj;
    }
}

static void op_add_array(cJSON *target, const char *key, int valid_inputs) {
    cJSON *arr = NULL;
    int size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    int size_after = 0;

    arr = cJSON_AddArrayToObject(target, key);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    assert_add_postconditions(target, key, valid_inputs, arr != NULL, size_before, size_after);

    if (arr != NULL) {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber(8.0));
        cJSON_AddItemToArray(arr, cJSON_CreateTrue());
    }
}

static void op_add_reference(cJSON *target, const char *key, int valid_inputs,
                             cJSON *shared_value) {
    int add_ok = 0;
    int size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    int size_after = 0;

    add_ok = cJSON_AddItemReferenceToObject(target, key, shared_value);
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    assert_add_postconditions(target, key, valid_inputs, add_ok, size_before, size_after);
}

static void op_add_item_raw(const uint8_t **data, size_t *size, cJSON *target, const char *key,
                            int valid_inputs) {
    cJSON *item = NULL;
    int add_ok = 0;
    int size_before = 0;
    int size_after = 0;

    item = create_add_item(data, size, 1);
    if (item == NULL) {
        return;
    }

    size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    add_ok = cJSON_AddItemToObject(target, key, item);
    if (!add_ok) {
        cJSON_Delete(item);
    }
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;

    assert_add_postconditions(target, key, valid_inputs, add_ok, size_before, size_after);
}

static void op_add_item_cs(cJSON *target, const char *key, int valid_inputs) {
    cJSON *item = cJSON_CreateNumber(7.0);
    int add_ok = 0;
    int size_before = 0;
    int size_after = 0;

    if (item == NULL) {
        return;
    }

    size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    add_ok = cJSON_AddItemToObjectCS(target, key, item);
    if (!add_ok) {
        cJSON_Delete(item);
    }
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;

    assert_add_postconditions(target, key, valid_inputs, add_ok, size_before, size_after);
}

static void op_add_detached(cJSON *root, cJSON *target, const char *key, int valid_inputs) {
    cJSON *tmp = cJSON_CreateNumber(1.0);
    cJSON *detached = NULL;
    int add_ok = 0;
    int size_before = 0;
    int size_after = 0;

    if (tmp == NULL) {
        return;
    }

    if (!cJSON_AddItemToObject(root, "oldkey", tmp)) {
        cJSON_Delete(tmp);
        return;
    }

    detached = cJSON_DetachItemFromObject(root, "oldkey");
    if (detached == NULL) {
        abort();
    }
    if (cJSON_GetObjectItemCaseSensitive(root, "oldkey") != NULL) {
        cJSON_Delete(detached);
        abort();
    }

    size_before = (target != NULL) ? cJSON_GetArraySize(target) : 0;
    add_ok = cJSON_AddItemToObject(target, key, detached);
    if (!add_ok) {
        cJSON_Delete(detached);
    }
    size_after = (target != NULL) ? cJSON_GetArraySize(target) : 0;

    assert_add_postconditions(target, key, valid_inputs, add_ok, size_before, size_after);
}

static void op_invalid_reference_array_call(const uint8_t **data, size_t *size, cJSON *shared_array_item) {
    uint8_t control = 0;
    cJSON *parent = NULL;
    int size_before = 0;
    int size_after = 0;
    int add_ok = 0;

    if (!read_u8(data, size, &control)) {
        return;
    }

    if ((control & 1) != 0) {
        parent = cJSON_CreateArray();
    }

    size_before = (parent != NULL) ? cJSON_GetArraySize(parent) : 0;
    add_ok = cJSON_AddItemReferenceToArray(parent, shared_array_item);
    size_after = (parent != NULL) ? cJSON_GetArraySize(parent) : 0;

    if (parent == NULL) {
        if (add_ok) {
            abort();
        }
    } else {
        if (!add_ok || size_after != (size_before + 1)) {
            cJSON_Delete(parent);
            abort();
        }
    }

    cJSON_Delete(parent);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    cJSON *root = NULL;
    cJSON *shared_value = NULL;
    cJSON *shared_array_item = NULL;
    cJSON *containers[8] = {0};
    size_t container_count = 0;
    uint8_t raw_ops = 0;
    size_t op_count = 0;

    if (data == NULL || size == 0) {
        return 0;
    }

    root = cJSON_CreateObject();
    if (root == NULL) {
        return 0;
    }

    seed_object(root, containers, &container_count);

    shared_value = cJSON_CreateString("shared");
    shared_array_item = cJSON_CreateNumber(1337.0);
    if (shared_value == NULL || shared_array_item == NULL) {
        cJSON_Delete(shared_value);
        cJSON_Delete(shared_array_item);
        cJSON_Delete(root);
        return 0;
    }

    if (!read_u8(&data, &size, &raw_ops)) {
        cJSON_Delete(shared_value);
        cJSON_Delete(shared_array_item);
        cJSON_Delete(root);
        return 0;
    }
    op_count = (size_t)(raw_ops % 32);

    for (size_t i = 0; i < op_count && size > 0; ++i) {
        uint8_t opcode = 0;
        uint8_t target_sel = 0;
        uint8_t fail_mode = 0;
        char *key_storage = NULL;
        const char *key = NULL;
        cJSON *target = NULL;
        int valid_inputs = 0;

        if (!read_u8(&data, &size, &opcode)) {
            break;
        }
        if (!read_u8(&data, &size, &target_sel)) {
            break;
        }
        if (!read_u8(&data, &size, &fail_mode)) {
            break;
        }

        if (container_count > 0) {
            target = containers[target_sel % container_count];
        }

        key_storage = choose_add_key(&data, &size);
        key = key_storage;

        if ((fail_mode & 0x3) == 0) {
            key = NULL;
        }
        if (((fail_mode >> 2) & 0x3) == 0) {
            target = NULL;
        }

        valid_inputs = (target != NULL && key != NULL);

        switch (opcode % 12) {
            case 0:
                op_add_number(&data, &size, target, key, valid_inputs);
                break;

            case 1:
                op_add_string(&data, &size, target, key, valid_inputs);
                break;

            case 2:
                op_add_raw(&data, &size, target, key, valid_inputs);
                break;

            case 3:
                op_add_true(target, key, valid_inputs);
                break;

            case 4:
                op_add_false(target, key, valid_inputs);
                break;

            case 5:
                op_add_null(target, key, valid_inputs);
                break;

            case 6:
                op_add_bool(&data, &size, target, key, valid_inputs);
                break;

            case 7:
                op_add_object(target, key, valid_inputs, containers, &container_count);
                break;

            case 8:
                op_add_array(target, key, valid_inputs);
                break;

            case 9:
                op_add_reference(target, key, valid_inputs, shared_value);
                break;

            case 10:
                op_add_item_raw(&data, &size, target, key, valid_inputs);
                break;

            case 11:
            default:
                if ((opcode & 1) == 0) {
                    op_add_item_cs(target, key, valid_inputs);
                } else {
                    op_add_detached(root, target, key, valid_inputs);
                }
                break;
        }

        op_invalid_reference_array_call(&data, &size, shared_array_item);
        maybe_rebuild(&data, &size, root, containers, &container_count);
        free(key_storage);
    }

    {
        char *printed = cJSON_PrintUnformatted(root);
        if (printed != NULL) {
            cJSON *parsed = cJSON_Parse(printed);
            if (parsed != NULL) {
                cJSON_Delete(parsed);
            }
            free(printed);
        }
    }

    cJSON_Delete(shared_value);
    cJSON_Delete(shared_array_item);
    cJSON_Delete(root);
    return 0;
}

#ifdef __cplusplus
}
#endif
