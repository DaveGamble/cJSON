// Copyright 2025 Google LLC
// Fuzz target for cJSON array operations
// Targets: cJSON_CreateStringArray, cJSON_Duplicate, cJSON_Compare

#include <cjson/cJSON.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Helper to safely create string array from fuzz input
cJSON* create_string_array_from_input(const char* data, size_t size) {
    // Parse input as newline-separated strings
    const char* strings[32];
    int count = 0;
    char* temp = (char*)malloc(size + 1);
    if (!temp) return NULL;
    
    memcpy(temp, data, size);
    temp[size] = '\0';
    
    char* token = strtok(temp, "\n");
    while (token && count < 32) {
        strings[count++] = token;
        token = strtok(NULL, "\n");
    }
    
    cJSON* arr = NULL;
    if (count > 0) {
        arr = cJSON_CreateStringArray(strings, count);
    }
    
    free(temp);
    return arr;
}

// Fuzz target for cJSON array and comparison operations
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;
    
    // Split input into two parts for comparison testing
    size_t split = size / 2;
    
    // Create first JSON object
    char* input1 = (char*)malloc(split + 1);
    if (!input1) return 0;
    memcpy(input1, data, split);
    input1[split] = '\0';
    
    cJSON* obj1 = cJSON_Parse(input1);
    free(input1);
    
    // Create second JSON object
    char* input2 = (char*)malloc(size - split + 1);
    if (!input2) {
        if (obj1) cJSON_Delete(obj1);
        return 0;
    }
    memcpy(input2, data + split, size - split);
    input2[size - split] = '\0';
    
    cJSON* obj2 = cJSON_Parse(input2);
    free(input2);
    
    // Test cJSON_Compare - 0% coverage target
    if (obj1 && obj2) {
        cJSON_Compare(obj1, obj2, cJSON_False);
        cJSON_Compare(obj1, obj2, cJSON_True); // case-sensitive
    }
    
    // Test cJSON_Duplicate - 0% coverage target
    if (obj1) {
        cJSON* dup1 = cJSON_Duplicate(obj1, cJSON_False); // shallow
        if (dup1) cJSON_Delete(dup1);
        
        cJSON* dup2 = cJSON_Duplicate(obj1, cJSON_True); // deep
        if (dup2) cJSON_Delete(dup2);
    }
    
    // Test cJSON_CreateStringArray - 0% coverage target
    cJSON* str_array = create_string_array_from_input((const char*)data, size);
    if (str_array) {
        // Test detaching and deleting items
        cJSON* detached = cJSON_DetachItemFromArray(str_array, 0);
        if (detached) cJSON_Delete(detached);
        
        cJSON_DeleteItemFromArray(str_array, 0);
        cJSON_Delete(str_array);
    }
    
    // Test object detachment functions
    if (obj1) {
        cJSON* detached = cJSON_DetachItemFromObject(obj1, "key");
        if (detached) cJSON_Delete(detached);
        
        cJSON_DeleteItemFromObject(obj1, "key");
        cJSON_DeleteItemFromObjectCaseSensitive(obj1, "Key");
    }
    
    // Cleanup
    if (obj1) cJSON_Delete(obj1);
    if (obj2) cJSON_Delete(obj2);
    
    return 0;
}
