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
  Fuzzer for the cJSON document-builder and modification API.

  Threat model: an application parses an untrusted JSON document
  (attacker-controlled, here segment 1) and then modifies it through the
  public mutation API with attacker-influenced member names, array
  indices and item documents (segments 2 and 3). The existing read
  fuzzer only covers parse, print and minify; roughly half of cJSON.c
  (the add/insert/replace/detach/delete list surgery, the create-family,
  the set-value helpers and the print-buffer growth paths) is dark in
  the production corpus. Those paths are exactly where list-surgery
  use-after-free, double-free and leak bugs live.

  API contracts honoured: AddItemToObject / InsertItemInArray /
  ReplaceItemInArray / ReplaceItemInObject keep ownership of the item on
  failure (the harness deletes it), the Add<type>ToObject helpers
  delete on failure internally, detached items are deleted by the
  harness, and SetValuestring / SetNumberHelper are only called on
  string / number typed nodes.

  Input layout (segments are NUL-separated, the last byte must be '\0'):
    data[0]  operation selector (modulo 8)
    data[1]  array index selector (0..63)
    data[2]  sub-selector: Add-family member for operation 5
    data[3]  bit0: case-sensitive variants
             bit1: formatted printing
             bit2: require_null_terminated parsing
    seg 1    base document (parsed with cJSON_ParseWithLengthOpts)
    seg 2    member name (raw bytes, used as C string)
    seg 3    item document
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size); /* required by C89 */

/* Extract the next NUL-terminated segment of the input. Returns the start
 * position of the following segment, or size when the input is exhausted.
 * The reported length excludes the terminator. */
static size_t next_segment(const uint8_t* data, size_t size, size_t start,
                           const char** segment, size_t* length)
{
    size_t position = start;

    while ((position < size) && (data[position] != '\0'))
    {
        position++;
    }

    if (position >= size)
    {
        *segment = NULL;
        *length = 0;
        return size;
    }

    *segment = (const char*)&data[start];
    *length = position - start;
    return position + 1;
}

/* Parse a segment with cJSON_ParseWithLengthOpts, including the segment
 * terminator in the buffer so require_null_terminated can succeed. */
static cJSON *parse_segment(const char *text, size_t length, cJSON_bool require_null_terminated)
{
    if (text == NULL)
    {
        return NULL;
    }

    return cJSON_ParseWithLengthOpts(text, length + 1, NULL, require_null_terminated);
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const char *base_text = NULL;
    const char *name_text = NULL;
    const char *item_text = NULL;
    size_t base_length = 0;
    size_t name_length = 0;
    size_t item_length = 0;
    size_t position = 0;
    cJSON *base = NULL;
    cJSON *item = NULL;
    cJSON *duplicate = NULL;
    cJSON *child = NULL;
    cJSON *detached = NULL;
    cJSON_bool case_sensitive = 0;
    cJSON_bool formatted = 0;
    cJSON_bool require_null_terminated = 0;
    int operation = 0;
    int index = 0;
    int sub_selector = 0;

    if (size < 10)
    {
        return 0;
    }
    if (data[size - 1] != '\0')
    {
        return 0;
    }

    operation = (int)(data[0] % 8);
    index = (int)(data[1] % 64);
    sub_selector = (int)(data[2] % 9);
    case_sensitive = (cJSON_bool)(data[3] & 1);
    formatted = (cJSON_bool)((data[3] >> 1) & 1);
    require_null_terminated = (cJSON_bool)((data[3] >> 2) & 1);

    position = next_segment(data, size, 4, &base_text, &base_length);
    position = next_segment(data, size, position, &name_text, &name_length);
    (void)next_segment(data, size, position, &item_text, &item_length);

    base = parse_segment(base_text, base_length, require_null_terminated);

    switch (operation)
    {
        case 0: /* add a parsed item to the base array */
        {
            if ((base != NULL) && cJSON_IsArray(base))
            {
                item = parse_segment(item_text, item_length, require_null_terminated);
                if ((item != NULL) && !cJSON_AddItemToArray(base, item))
                {
                    cJSON_Delete(item);
                }
            }
            break;
        }

        case 1: /* insert a parsed item at a fuzzed index */
        {
            if ((base != NULL) && cJSON_IsArray(base))
            {
                item = parse_segment(item_text, item_length, require_null_terminated);
                if ((item != NULL) && !cJSON_InsertItemInArray(base, index, item))
                {
                    cJSON_Delete(item);
                }
            }
            break;
        }

        case 2: /* replace a fuzzed array index */
        {
            if ((base != NULL) && cJSON_IsArray(base))
            {
                item = parse_segment(item_text, item_length, require_null_terminated);
                if ((item != NULL) && !cJSON_ReplaceItemInArray(base, index, item))
                {
                    cJSON_Delete(item);
                }
            }
            break;
        }

        case 3: /* add a parsed item under a fuzzed member name */
        {
            if ((base != NULL) && cJSON_IsObject(base) && (name_text != NULL))
            {
                item = parse_segment(item_text, item_length, require_null_terminated);
                if ((item != NULL) && !cJSON_AddItemToObject(base, name_text, item))
                {
                    cJSON_Delete(item);
                }
            }
            break;
        }

        case 4: /* replace a fuzzed member name */
        {
            if ((base != NULL) && cJSON_IsObject(base) && (name_text != NULL))
            {
                item = parse_segment(item_text, item_length, require_null_terminated);
                if (item != NULL)
                {
                    cJSON_bool replaced = case_sensitive
                        ? cJSON_ReplaceItemInObjectCaseSensitive(base, name_text, item)
                        : cJSON_ReplaceItemInObject(base, name_text, item);
                    if (!replaced)
                    {
                        cJSON_Delete(item);
                    }
                }
            }
            break;
        }

        case 5: /* the Add<type>ToObject creation matrix */
        {
            if ((base != NULL) && cJSON_IsObject(base) && (name_text != NULL))
            {
                switch (sub_selector)
                {
                    case 0:
                        (void)cJSON_AddNullToObject(base, name_text);
                        break;
                    case 1:
                        (void)cJSON_AddTrueToObject(base, name_text);
                        break;
                    case 2:
                        (void)cJSON_AddFalseToObject(base, name_text);
                        break;
                    case 3:
                        (void)cJSON_AddBoolToObject(base, name_text,
                                                    (cJSON_bool)((data[3] >> 3) & 1));
                        break;
                    case 4:
                        (void)cJSON_AddNumberToObject(base, name_text,
                                                      -1.5 * (double)index - 0.25);
                        break;
                    case 5:
                        (void)cJSON_AddStringToObject(base, name_text, name_text);
                        break;
                    case 6:
                        (void)cJSON_AddRawToObject(base, name_text, "[1, 2, 3]");
                        break;
                    case 7:
                        (void)cJSON_AddObjectToObject(base, name_text);
                        break;
                    default:
                        (void)cJSON_AddArrayToObject(base, name_text);
                        break;
                }
            }
            break;
        }

        case 6: /* detach / delete from the base document */
        {
            if (base != NULL)
            {
                if (cJSON_IsArray(base))
                {
                    detached = cJSON_DetachItemFromArray(base, index);
                    cJSON_Delete(detached);
                }
                else if (cJSON_IsObject(base) && (name_text != NULL))
                {
                    if (case_sensitive)
                    {
                        cJSON_DeleteItemFromObjectCaseSensitive(base, name_text);
                    }
                    else
                    {
                        cJSON_DeleteItemFromObject(base, name_text);
                    }
                }
            }
            break;
        }

        default: /* set values on an existing member, then compare a duplicate */
        {
            if ((base != NULL) && cJSON_IsObject(base) && (name_text != NULL))
            {
                child = case_sensitive
                    ? cJSON_GetObjectItemCaseSensitive(base, name_text)
                    : cJSON_GetObjectItem(base, name_text);
                (void)cJSON_HasObjectItem(base, name_text);
                if (child != NULL)
                {
                    if (cJSON_IsString(child))
                    {
                        (void)cJSON_SetValuestring(child, "fuzzed string value");
                    }
                    if (cJSON_IsNumber(child))
                    {
                        (void)cJSON_SetNumberHelper(child, -1.5 * (double)index - 0.25);
                    }
                }

                duplicate = cJSON_Duplicate(base, 1);
                if (duplicate != NULL)
                {
                    (void)cJSON_Compare(base, duplicate, case_sensitive);
                    cJSON_Delete(duplicate);
                }
            }
            break;
        }
    }

    if (base != NULL)
    {
        char *printed = cJSON_PrintBuffered(base, 1, formatted);
        cJSON_free(printed);

        printed = cJSON_PrintUnformatted(base);
        if (printed != NULL)
        {
            cJSON_Minify(printed);
            cJSON_free(printed);
        }
    }

    cJSON_Delete(base);

    return 0;
}

#ifdef __cplusplus
}
#endif