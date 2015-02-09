#include "cJSON.h"

// Implement RFC6901 (https://tools.ietf.org/html/rfc6901) JSON Pointer spec.
cJSON *cJSONUtils_GetPointer(cJSON *object,const char *pointer);

