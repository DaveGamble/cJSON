#include <stdlib.h>
#include <stdint.h>

#include "../cJSON.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if((data[0] == '\0') || (size < 3) || (data[1] == '\0')) return 0;

    cJSON *json = cJSON_Parse((const char*)data + 2);

    if(json == NULL) return 0;

    int do_format = 0;
    char *printed_json = NULL;

    if(data[1] == 'f') do_format = 1;

    if(data[0] == 'b')
    {
        /* buffered printing */
        printed_json = cJSON_PrintBuffered(json, 1, do_format);
    }
    else
    {
        /* unbuffered printing */
        if(do_format)
        {
            printed_json = cJSON_Print(json);
        }
        else
        {
            printed_json = cJSON_PrintUnformatted(json);
        }
    }

    if(printed_json != NULL) free(printed_json);
    cJSON_Delete(json);

    return 0;
}
