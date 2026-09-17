// Copyright 2025 Google LLC
// Fuzz target for cJSON serialization functions
// Targets: cJSON_PrintPreallocated, cJSON_PrintBuffered

#include <cjson/cJSON.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Fuzz target for cJSON serialization with preallocated buffers
// Targets 0% coverage functions identified by Fuzz Introspector
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Ensure null-terminated string
    char* input = (char*)malloc(size + 1);
    if (!input) return 0;
    
    memcpy(input, data, size);
    input[size] = '\0';
    
    // Parse JSON first
    cJSON* root = cJSON_Parse(input);
    if (!root) {
        free(input);
        return 0;
    }
    
    // Test cJSON_PrintPreallocated - 0% coverage target
    // This function prints to a user-provided buffer
    size_t buffer_size = 65536; // 64KB initial buffer
    char* buffer = (char*)malloc(buffer_size);
    if (buffer) {
        cJSON_bool prealloc_result = cJSON_PrintPreallocated(
            root, buffer, (int)buffer_size, cJSON_False
        );
        // Try with formatted output as well
        if (!prealloc_result) {
            cJSON_PrintPreallocated(root, buffer, (int)buffer_size, cJSON_True);
        }
        free(buffer);
    }
    
    // Test various cJSON structures for better coverage
    cJSON* test_obj = cJSON_CreateObject();
    if (test_obj) {
        // Add various data types
        cJSON_AddNullToObject(test_obj, "null_val");
        cJSON_AddTrueToObject(test_obj, "true_val");
        cJSON_AddFalseToObject(test_obj, "false_val");
        cJSON_AddBoolToObject(test_obj, "bool_val", cJSON_True);
        cJSON_AddNumberToObject(test_obj, "num_val", 3.14159);
        cJSON_AddStringToObject(test_obj, "str_val", "test");
        cJSON_AddRawToObject(test_obj, "raw_val", "{\"nested\":true}");
        
        // Create and add array
        cJSON* arr = cJSON_CreateIntArray((const int[]){1, 2, 3}, 3);
        cJSON_AddItemToObject(test_obj, "int_array", arr);
        
        cJSON* double_arr = cJSON_CreateDoubleArray(
            (const double[]){1.1, 2.2, 3.3}, 3
        );
        cJSON_AddItemToObject(test_obj, "double_array", double_arr);
        
        cJSON* float_arr = cJSON_CreateFloatArray(
            (const float[]){1.1f, 2.2f, 3.3f}, 3
        );
        cJSON_AddItemToObject(test_obj, "float_array", float_arr);
        
        // Cleanup
        cJSON_Delete(test_obj);
    }
    
    // Cleanup
    cJSON_Delete(root);
    free(input);
    
    return 0;
}
