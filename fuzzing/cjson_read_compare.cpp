#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fuzzer/FuzzedDataProvider.h>

extern "C" {
#include "../cJSON.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    cJSON *a_json, *b_json;
    FuzzedDataProvider fdp(data, size);

    std::string payload1 = fdp.ConsumeRandomLengthString();
    std::string payload2 = fdp.ConsumeRandomLengthString();

    a_json = cJSON_ParseWithOpts(payload1.c_str(), NULL, 1);
    b_json = cJSON_ParseWithOpts(payload2.c_str(), NULL, 1);

    if(a_json != NULL && b_json != NULL) {
        cJSON_Compare(a_json, b_json, fdp.ConsumeBool());
    }

    if (a_json != NULL) {
        cJSON_Delete(a_json);
    }
    if (b_json != NULL) {
        cJSON_Delete(b_json);
    }

    return 0;
}
