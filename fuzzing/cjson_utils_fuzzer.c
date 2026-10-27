#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../cJSON.h"
#include "../cJSON_Utils.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size); /* required by C89 */

/* The input is two NUL-terminated strings: a JSON document and a second
 * string that is used both as a JSON document and as a JSON pointer into
 * the first. */
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const char *first;
    const char *second;
    const unsigned char *separator;
    cJSON *from;
    cJSON *to;
    cJSON *copy;
    cJSON *patches;
    cJSON *pointed;
    char *pointer;

    if(size < 2) return 0;
    if(data[size-1] != '\0') return 0;
    separator = (const unsigned char*)memchr(data, '\0', size);
    if(separator == data + size - 1) return 0;

    first = (const char*)data;
    second = (const char*)separator + 1;

    from = cJSON_Parse(first);
    if(from == NULL) return 0;

    cJSONUtils_GetPointer(from, second);
    cJSONUtils_GetPointerCaseSensitive(from, second);

    /* the item the pointer selects, as the second argument of the functions
     * that take two documents: it is part of the first */
    pointed = cJSONUtils_GetPointer(from, second);
    if(pointed != NULL)
    {
        pointer = cJSONUtils_FindPointerFromObjectTo(from, pointed);
        if(pointer != NULL) free(pointer);
        patches = cJSONUtils_GeneratePatches(from, pointed);
        cJSON_Delete(patches);
        patches = cJSONUtils_GenerateMergePatch(from, pointed);
        cJSON_Delete(patches);

        copy = cJSON_Duplicate(from, 1);
        copy = cJSONUtils_MergePatch(copy, cJSONUtils_GetPointer(copy, second));
        cJSON_Delete(copy);
        copy = cJSON_Duplicate(from, 1);
        cJSONUtils_ApplyPatches(copy, cJSONUtils_GetPointer(copy, second));
        cJSON_Delete(copy);
    }

    to = cJSON_Parse(second);
    if(to == NULL)
    {
        cJSON_Delete(from);
        return 0;
    }

    /* the second document as a list of RFC 6902 patches */
    copy = cJSON_Duplicate(from, 1);
    cJSONUtils_ApplyPatches(copy, to);
    cJSON_Delete(copy);
    copy = cJSON_Duplicate(from, 1);
    cJSONUtils_ApplyPatchesCaseSensitive(copy, to);
    cJSON_Delete(copy);

    /* generated patches must apply */
    patches = cJSONUtils_GeneratePatches(from, to);
    copy = cJSON_Duplicate(from, 1);
    cJSONUtils_ApplyPatches(copy, patches);
    cJSON_Delete(copy);
    cJSON_Delete(patches);
    patches = cJSONUtils_GeneratePatchesCaseSensitive(from, to);
    cJSON_Delete(patches);

    /* the second document as an RFC 7386 merge patch */
    patches = cJSONUtils_GenerateMergePatch(from, to);
    cJSON_Delete(patches);
    patches = cJSONUtils_GenerateMergePatchCaseSensitive(from, to);
    cJSON_Delete(patches);
    copy = cJSONUtils_MergePatch(cJSON_Duplicate(from, 1), to);
    cJSON_Delete(copy);
    copy = cJSONUtils_MergePatchCaseSensitive(cJSON_Duplicate(from, 1), to);
    cJSON_Delete(copy);

    cJSONUtils_SortObject(from);
    cJSONUtils_SortObjectCaseSensitive(to);

    cJSON_Delete(from);
    cJSON_Delete(to);

    return 0;
}

#ifdef __cplusplus
}
#endif
