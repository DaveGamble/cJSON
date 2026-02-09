#include <fuzzer/FuzzedDataProvider.h>
#include <string>

extern "C" {
  #include "../cJSON.c"
}

int intArray[10];
float floatArray[10];
double doubleArray[10];
const char *stringArray[10];

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  std::string json_string = fdp.ConsumeRandomLengthString();
  cJSON *json = cJSON_ParseWithOpts(json_string.c_str(), NULL, fdp.ConsumeBool());

  if (json != NULL) {

    cJSON *new_json = cJSON_Duplicate(json, fdp.ConsumeBool());
    if (new_json != NULL) {
        cJSON_Delete(new_json);
    }

    // Modifify the object
    int opsToPerform = fdp.ConsumeIntegralInRange(5, 20);
    for (int i = 0; i < opsToPerform; i++) {
      int op = fdp.ConsumeIntegralInRange(0, 13);
      switch (op) {
        case 0:
          cJSON_AddBoolToObject(json, fdp.ConsumeRandomLengthString().c_str(), fdp.ConsumeBool());
          break;
        case 1:
          cJSON_AddTrueToObject(json, fdp.ConsumeRandomLengthString().c_str());
          break;
        case 2:
          cJSON_AddNumberToObject(json, fdp.ConsumeRandomLengthString().c_str(), 1.0);
          break;
        case 3:
          cJSON_GetObjectItem(json, fdp.ConsumeRandomLengthString().c_str());
          break;
        case 4:
          cJSON_GetObjectItemCaseSensitive(json, fdp.ConsumeRandomLengthString().c_str());
          break;
        case 5:
          cJSON_AddNullToObject(json, fdp.ConsumeRandomLengthString().c_str());
          break;
        case 6:
          cJSON_AddStringToObject(json, fdp.ConsumeRandomLengthString().c_str(), fdp.ConsumeRandomLengthString().c_str());
          break;
        case 7:
          cJSON_AddRawToObject(json, fdp.ConsumeRandomLengthString().c_str(),  fdp.ConsumeRandomLengthString().c_str());
          break;
        case 8:
          cJSON_AddArrayToObject(json, fdp.ConsumeRandomLengthString().c_str());
          break;
        case 9:
          cJSON_AddFalseToObject(json, fdp.ConsumeRandomLengthString().c_str());
          break;
        case 10:
          cJSON_AddObjectToObject(json, fdp.ConsumeRandomLengthString().c_str());
          break;
        case 11:
          cJSON_SetNumberHelper(json, fdp.ConsumeFloatingPoint<double>());
          break;
        case 12:
          cJSON_SetValuestring(json, fdp.ConsumeRandomLengthString().c_str());
          break;                    
        default:
          break;
      }
    }

    // Array creation routines.
    for (int i = 0; i < 10; i++) {
      intArray[i] = fdp.ConsumeIntegral<int>();
      floatArray[i] = fdp.ConsumeFloatingPoint<float>();
      doubleArray[i] = fdp.ConsumeFloatingPoint<double>();
      stringArray[i] = json_string.c_str();
    }
    cJSON *cJsonIntArray = cJSON_CreateIntArray(intArray, 10);
    if (cJsonIntArray != NULL) {
      cJSON_Delete(cJsonIntArray);
    }
    cJSON *floatIntArray = cJSON_CreateFloatArray(floatArray, 10);
    if (floatIntArray != NULL) {
      cJSON_Delete(floatIntArray);
    }
    cJSON *cJsonDoubleArray = cJSON_CreateDoubleArray(doubleArray, 10);
    if (cJsonDoubleArray != NULL) {
      cJSON_Delete(cJsonDoubleArray);
    }        
    cJSON *cJsonStringArray = cJSON_CreateStringArray(stringArray, 10);
    if (cJsonStringArray != NULL) {
      cJSON_Delete(cJsonStringArray);
    }        

    // Replace
    cJSON *nullObj = cJSON_CreateNull();
    if (nullObj != NULL) {
      if (cJSON_ReplaceItemInObject(json, fdp.ConsumeRandomLengthString().c_str(), nullObj) == false) {
        cJSON_Delete(nullObj);
      }
    }
    // Print the object
    unsigned char printed_unformatted[1024];
    unsigned char printed_formatted[1024];
    printbuffer unformatted_buffer = { 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };

    /* buffer for formatted printing */
    unformatted_buffer.buffer = printed_unformatted;
    unformatted_buffer.length = sizeof(printed_unformatted);
    unformatted_buffer.offset = 0;
    unformatted_buffer.noalloc = true;
    unformatted_buffer.hooks = global_hooks;
    print_object(json, &unformatted_buffer);

    // Type checks
    opsToPerform = fdp.ConsumeIntegralInRange(5, 20);
    for (int i = 0; i < opsToPerform; i++) {
      int op = fdp.ConsumeIntegralInRange(0, 9);
      switch (op) {
        case 0:
          if (cJSON_IsArray(json)) {
            cJSON_GetArraySize(json);
            cJSON_GetArrayItem(json, 10);
          }
          break;
        case 1:
          cJSON_IsObject(json);
          break;
        case 2:
          cJSON_IsString(json);
          break;
        case 3:
          cJSON_IsRaw(json);
          break;
        case 4:
          cJSON_IsNull(json);
          break;
        case 5:
          cJSON_IsBool(json);
          break;
        case 6:
          cJSON_IsTrue(json);
          break;
        case 7:
          cJSON_IsFalse(json);
          break;
        case 8:
          cJSON_IsInvalid(json);
          break;                 
        default:
          break;
      }
    }
  }

  cJSON_Delete(json);
  return 0;
}
