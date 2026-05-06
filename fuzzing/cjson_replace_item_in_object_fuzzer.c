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

static char *read_key(const uint8_t **data, size_t *size) {
    uint8_t raw_len = 0;
    if (!read_u8(data, size, &raw_len)) {
        return NULL;
    }

    size_t key_len = (size_t)(raw_len % 64);
    if (*size < key_len) {
        key_len = *size;
    }

    char *key = (char *)malloc(key_len + 1);
    if (key == NULL) {
        return NULL;
    }

    if (key_len > 0) {
        memcpy(key, *data, key_len);
        (*data) += key_len;
        (*size) -= key_len;
    }
    key[key_len] = '\0';

    for (size_t i = 0; i < key_len; ++i) {
        if (key[i] == '\0') {
            key[i] = 'A';
        }
    }

    return key;
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

static void mutate_case(char *s, uint8_t mode) {
    if (s == NULL) {
        return;
    }

    for (size_t i = 0; s[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') {
            if (mode == 1 || (mode == 2 && (i % 2 == 0))) {
                s[i] = (char)(c - 'a' + 'A');
            }
        } else if (c >= 'A' && c <= 'Z') {
            if (mode == 3 || (mode == 2 && (i % 2 == 1))) {
                s[i] = (char)(c - 'A' + 'a');
            }
        }
    }
}

static cJSON *lookup_object_item(cJSON *object, const char *key, int case_sensitive) {
    if (case_sensitive) {
        return cJSON_GetObjectItemCaseSensitive(object, key);
    }
    return cJSON_GetObjectItem(object, key);
}

static cJSON *make_leaf_item(const uint8_t **data, size_t *size, uint8_t kind) {
    switch (kind % 4) {
        case 0: {
            double number = 0.0;
            (void)read_double(data, size, &number);
            return cJSON_CreateNumber(number);
        }
        case 1: {
            char *s = read_key(data, size);
            cJSON *item = cJSON_CreateString((s != NULL) ? s : "");
            free(s);
            return item;
        }
        case 2: {
            uint8_t b = 0;
            (void)read_u8(data, size, &b);
            return cJSON_CreateBool((b & 1) ? 1 : 0);
        }
        case 3:
        default:
            return cJSON_CreateNull();
    }
}

static cJSON *make_replacement_item(const uint8_t **data, size_t *size, int depth) {
    uint8_t kind = 0;
    if (!read_u8(data, size, &kind)) {
        return cJSON_CreateNull();
    }

    if (depth <= 0) {
        return make_leaf_item(data, size, kind);
    }

    switch (kind % 6) {
        case 0:
        case 1:
        case 2:
        case 3:
            return make_leaf_item(data, size, kind);

        case 4: {
            cJSON *arr = cJSON_CreateArray();
            if (arr == NULL) {
                return NULL;
            }

            uint8_t count = 0;
            (void)read_u8(data, size, &count);
            size_t n = (size_t)(count % 4);

            for (size_t i = 0; i < n; ++i) {
                cJSON *elem = make_replacement_item(data, size, depth - 1);
                if (elem == NULL || !cJSON_AddItemToArray(arr, elem)) {
                    cJSON_Delete(elem);
                    cJSON_Delete(arr);
                    return NULL;
                }
            }
            return arr;
        }

        case 5:
        default: {
            cJSON *obj = cJSON_CreateObject();
            if (obj == NULL) {
                return NULL;
            }

            uint8_t count = 0;
            (void)read_u8(data, size, &count);
            size_t n = (size_t)(count % 3);

            for (size_t i = 0; i < n; ++i) {
                char *k = read_key(data, size);
                if (k == NULL) {
                    break;
                }

                cJSON *val = make_replacement_item(data, size, depth - 1);
                if (val == NULL || !cJSON_AddItemToObject(obj, k, val)) {
                    cJSON_Delete(val);
                    free(k);
                    cJSON_Delete(obj);
                    return NULL;
                }
                free(k);
            }
            return obj;
        }
    }
}

static void seed_object(cJSON *root) {
    cJSON_AddNumberToObject(root, "alpha", 1.0);
    cJSON_AddNumberToObject(root, "Bravo", 2.0);
    cJSON_AddNumberToObject(root, "CHARLIE", 3.0);
    cJSON_AddStringToObject(root, "delta", "x");
    cJSON_AddBoolToObject(root, "Echo", 1);
    cJSON_AddNullToObject(root, "foxtrot");
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (data == NULL || size == 0) {
        return 0;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return 0;
    }

    seed_object(root);

    uint8_t raw_ops = *data++;
    size--;

    size_t op_count = (size_t)(raw_ops % 32);

    for (size_t i = 0; i < op_count && size > 0; ++i) {
        char *lookup_key = read_key(&data, &size);
        int case_sensitive = 0;
        cJSON *existing = NULL;
        int size_before = 0;
        int replace_ok = 0;
        int size_after = 0;
        cJSON *after_item = NULL;
        if (lookup_key == NULL) {
            break;
        }

        uint8_t case_mode = 0;
        (void)read_u8(&data, &size, &case_mode);
        mutate_case(lookup_key, (uint8_t)(case_mode % 4));

        cJSON *replacement = make_replacement_item(&data, &size, 1);
        if (replacement == NULL) {
            free(lookup_key);
            break;
        }

        uint8_t which_api = 0;
        (void)read_u8(&data, &size, &which_api);
        case_sensitive = (which_api & 1) != 0;
        existing = lookup_object_item(root, lookup_key, case_sensitive);
        size_before = cJSON_GetArraySize(root);

        if (!case_sensitive) {
            replace_ok = cJSON_ReplaceItemInObject(root, lookup_key, replacement);
            if (!replace_ok) {
                cJSON_Delete(replacement);
            }
        } else {
            replace_ok = cJSON_ReplaceItemInObjectCaseSensitive(root, lookup_key, replacement);
            if (!replace_ok) {
                cJSON_Delete(replacement);
            }
        }

        size_after = cJSON_GetArraySize(root);
        after_item = lookup_object_item(root, lookup_key, case_sensitive);

        if (existing != NULL) {
            /* Semantic/metamorphic oracle: successful replace preserves size and keeps the key reachable. */
            if (!replace_ok || size_after != size_before || after_item == NULL) {
                free(lookup_key);
                abort();
            }
        } else {
            /* Semantic/metamorphic oracle: replacing a missing key should fail without changing object size. */
            if (replace_ok || size_after != size_before) {
                free(lookup_key);
                abort();
            }
        }

        free(lookup_key);
    }

    char *printed = cJSON_PrintUnformatted(root);
    if (printed != NULL) {
        cJSON *parsed = cJSON_Parse(printed);
        if (parsed != NULL) {
            cJSON_Delete(parsed);
        }
        free(printed);
    }

    cJSON_Delete(root);
    return 0;
}

#ifdef __cplusplus
}
#endif
