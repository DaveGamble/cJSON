#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../cJSON.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    size_t offset = 4;

    if(size < offset) return 0;
    if(data[0] != '1' && data[0] != '0') return 0;
    if(data[1] != '1' && data[1] != '0') return 0;
    if(data[2] != '1' && data[2] != '0') return 0;
    if(data[3] != '1' && data[3] != '0') return 0;

    int minify              = data[0] == '1' ? 1 : 0;
    int require_termination = data[1] == '1' ? 1 : 0;
    int formatted           = data[2] == '1' ? 1 : 0;
    int buffered            = data[3] == '1' ? 1 : 0;

    unsigned char *copied = (unsigned char*)malloc(size);
    if(copied == NULL) return 0;

    memcpy(copied, data, size);
    copied[size-1] = '\0';

    cJSON *json = cJSON_ParseWithOpts((const char*)copied + offset, NULL, require_termination);

    if(json == NULL)
    {
        free(copied);
        return 0;
    }

    char *printed_json = NULL;

    if(buffered)
    {
        printed_json = cJSON_PrintBuffered(json, 1, formatted);
    }
    else
    {
        /* unbuffered printing */
        if(formatted)
        {
            printed_json = cJSON_Print(json);
        }
        else
        {
            printed_json = cJSON_PrintUnformatted(json);
        }
    }

    if(printed_json != NULL) free(printed_json);

    if(minify)
    {
        cJSON_Minify((char*)copied + offset);
    }

    cJSON_Delete(json);
    free(copied);

    return 0;
}
