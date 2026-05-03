// Copyright 2025 Google LLC
// Fuzz target for cJSON object manipulation functions
// Targets: cJSON_Parse, cJSON_ReplaceItemInObject, cJSON_AddItemToObject

#include <cjson/cJSON.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Fuzz target for cJSON object manipulation
// This targets functions that currently have 0% coverage according to Fuzz Introspector
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Ensure null-terminated string for JSON parsing
    char* input = (char*)malloc(size + 1);
    if (!input) return 0;
    
    memcpy(input, data, size);
    input[size] = '\0';
    
    // Parse the input as JSON
    cJSON* root = cJSON_Parse(input);
    if (!root) {
        free(input);
        return 0;
    }
    
    // Test cJSON_ReplaceItemInObject - 0% coverage target
    cJSON* replacement = cJSON_CreateString("fuzz_value");
    if (replacement) {
        cJSON_ReplaceItemInObject(root, "key", replacement);
        // Also test case-sensitive version
        cJSON_ReplaceItemInObjectCaseSensitive(root, "Key", replacement);
    }
    
    // Test cJSON_AddItemToObject - another 0% coverage target
    cJSON* new_item = cJSON_CreateNumber(42);
    if (new_item) {
        cJSON_AddItemToObject(root, "fuzz_key", new_item);
    }
    
    // Test cJSON_AddItemToObjectCS (constant string key)
    cJSON* cs_item = cJSON_CreateBool(1);
    if (cs_item) {
        cJSON_AddItemToObjectCS(root, "const_key", cs_item);
    }
    
    // Test array manipulation
    cJSON* array = cJSON_GetObjectItem(root, "array");
    if (array && cJSON_IsArray(array)) {
        cJSON* arr_item = cJSON_CreateNumber(123);
        cJSON_ReplaceItemInArray(array, 0, arr_item);
    }
    
    // Cleanup
    cJSON_Delete(root);
    free(input);
    
    return 0;
}
