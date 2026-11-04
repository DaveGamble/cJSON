/*
  Copyright (c) 2009-2026 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/*
  Fuzzer for cJSON_Utils: RFC 6901 JSON Pointer, RFC 6902 JSON Patch and
  RFC 7396 JSON Merge Patch.

  Threat model: applications routinely hand untrusted JSON documents to
  cJSON_Utils (configuration stores, document databases, HTTP PATCH and
  JSON-merge endpoints). The attacker controls the target document, the
  patch document and the raw pointer text; this harness derives all three
  from the fuzz input. The module's recent memory-safety history
  (heap-use-after-free in merge_patch when the patch is a subtree of the
  target, type confusion on patch values) is the bug-class mix this
  harness guards: use-after-free, type confusion, NULL dereference and
  leaks on attacker-shaped documents.

  Input layout (segments are NUL-separated, the last byte must be '\0'):
    data[0]  bit0: use the CaseSensitive API variants
             bit1: formatted printing
    data[1]  operation selector (modulo 6)
    data[2]  patch "op" keyword selector for the AddPatchToArray operation
    seg 1    document A (target document)
    seg 2    document B (patch / second document)
    seg 3    raw pointer text, also parsed as patch document C
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../cJSON.h"
#include "../cJSON_Utils.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size); /* required by C89 */

static const char *patch_operations[6] =
{
    "add", "remove", "replace", "move", "copy", "test"
};

/* Extract the next NUL-terminated segment of the input. Returns the start
 * position of the following segment, or size when the input is exhausted. */
static size_t next_segment(const uint8_t* data, size_t size, size_t start, const char** segment)
{
    size_t position = start;

    while ((position < size) && (data[position] != '\0'))
    {
        position++;
    }

    if (position >= size)
    {
        *segment = NULL;
        return size;
    }

    *segment = (const char*)&data[start];
    return position + 1;
}

static void print_document(const cJSON *document, cJSON_bool formatted)
{
    char *printed = NULL;

    printed = cJSON_PrintBuffered(document, 1, formatted);
    cJSON_free(printed);

    printed = cJSON_PrintUnformatted(document);
    cJSON_free(printed);
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const char *document_a_text = NULL;
    const char *document_b_text = NULL;
    const char *pointer_text = NULL;
    size_t position = 0;
    cJSON *document_a = NULL;
    cJSON *document_b = NULL;
    cJSON *document_c = NULL;
    cJSON *found = NULL;
    cJSON_bool case_sensitive = 0;
    cJSON_bool formatted = 0;
    int operation = 0;
    int patch_operation = 0;

    if (size < 8)
    {
        return 0;
    }
    if (data[size - 1] != '\0')
    {
        return 0;
    }

    case_sensitive = (cJSON_bool)(data[0] & 1);
    formatted = (cJSON_bool)((data[0] >> 1) & 1);
    operation = (int)(data[1] % 6);
    patch_operation = (int)(data[2] % 6);

    position = next_segment(data, size, 3, &document_a_text);
    position = next_segment(data, size, position, &document_b_text);
    (void)next_segment(data, size, position, &pointer_text);

    document_a = cJSON_Parse(document_a_text);
    document_b = cJSON_Parse(document_b_text);
    document_c = cJSON_Parse(pointer_text);

    switch (operation)
    {
        case 0: /* RFC 6902: apply the patch array from the third segment */
        {
            if ((document_a != NULL) && (document_c != NULL))
            {
                if (case_sensitive)
                {
                    (void)cJSONUtils_ApplyPatchesCaseSensitive(document_a, document_c);
                }
                else
                {
                    (void)cJSONUtils_ApplyPatches(document_a, document_c);
                }
            }
            break;
        }

        case 1: /* RFC 7396: merge patch; target ownership moves to the result */
        {
            if (document_a != NULL)
            {
                if (case_sensitive)
                {
                    document_a = cJSONUtils_MergePatchCaseSensitive(document_a, document_b);
                }
                else
                {
                    document_a = cJSONUtils_MergePatch(document_a, document_b);
                }
            }
            break;
        }

        case 2: /* generate a patch between the documents, then apply it */
        {
            if ((document_a != NULL) && (document_b != NULL))
            {
                cJSON *from = cJSON_Duplicate(document_a, 1);
                cJSON *to = cJSON_Duplicate(document_b, 1);

                if ((from != NULL) && (to != NULL))
                {
                    cJSON *generated = NULL;

                    /* GeneratePatches sorts 'from' and 'to' in place. */
                    if (case_sensitive)
                    {
                        generated = cJSONUtils_GeneratePatchesCaseSensitive(from, to);
                    }
                    else
                    {
                        generated = cJSONUtils_GeneratePatches(from, to);
                    }

                    if (generated != NULL)
                    {
                        if (case_sensitive)
                        {
                            (void)cJSONUtils_ApplyPatchesCaseSensitive(document_a, generated);
                        }
                        else
                        {
                            (void)cJSONUtils_ApplyPatches(document_a, generated);
                        }
                        print_document(generated, formatted);
                        cJSON_Delete(generated);
                    }
                }
                cJSON_Delete(from);
                cJSON_Delete(to);
            }
            break;
        }

        case 3: /* RFC 6901: pointer lookup, pointer construction and sorting */
        {
            if (document_a != NULL)
            {
                char *constructed = NULL;

                found = case_sensitive
                    ? cJSONUtils_GetPointerCaseSensitive(document_a, pointer_text)
                    : cJSONUtils_GetPointer(document_a, pointer_text);
                if (found != NULL)
                {
                    constructed = cJSONUtils_FindPointerFromObjectTo(document_a, found);
                    cJSON_free(constructed);
                }
                /* the sort API is documented for object members */
                if (cJSON_IsObject(document_a))
                {
                    if (case_sensitive)
                    {
                        cJSONUtils_SortObjectCaseSensitive(document_a);
                    }
                    else
                    {
                        cJSONUtils_SortObject(document_a);
                    }
                }
            }
            break;
        }

        case 4: /* merge with a patch that is a subtree of the target; guards
                 * the heap-use-after-free fixed in upstream patch #1065.
                 * Self-merge (patch == target, e.g. the RFC 6901 empty
                 * pointer resolving to the root) is routed around: it is
                 * a known unresolved heap-use-after-free upstream, see
                 * FINDING-1 in the run findings. */
        {
            if (document_a != NULL)
            {
                found = case_sensitive
                    ? cJSONUtils_GetPointerCaseSensitive(document_a, pointer_text)
                    : cJSONUtils_GetPointer(document_a, pointer_text);
                if ((found != NULL) && (found != document_a))
                {
                    if (case_sensitive)
                    {
                        document_a = cJSONUtils_MergePatchCaseSensitive(document_a, found);
                    }
                    else
                    {
                        document_a = cJSONUtils_MergePatch(document_a, found);
                    }
                }
            }
            break;
        }

        default: /* build a patch entry with AddPatchToArray, then apply it */
        {
            if ((document_a != NULL) && (pointer_text != NULL))
            {
                cJSON *built = cJSON_CreateArray();

                if (built != NULL)
                {
                    cJSONUtils_AddPatchToArray(built, patch_operations[patch_operation],
                                               pointer_text, document_b);
                    if (case_sensitive)
                    {
                        (void)cJSONUtils_ApplyPatchesCaseSensitive(document_a, built);
                    }
                    else
                    {
                        (void)cJSONUtils_ApplyPatches(document_a, built);
                    }
                    print_document(built, formatted);
                    cJSON_Delete(built);
                }
            }
            break;
        }
    }

    if (document_a != NULL)
    {
        print_document(document_a, formatted);
    }

    cJSON_Delete(document_a);
    cJSON_Delete(document_b);
    cJSON_Delete(document_c);

    return 0;
}

#ifdef __cplusplus
}
#endif