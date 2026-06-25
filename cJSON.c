/*
 * Copyright (c) 2009-2017 Dave Gamble and cJSON contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * JSON parser in C.
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning(push)
#pragma warning(disable: 4001)
#endif

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>

#ifdef ENABLE_LOCALES
#include <locale.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC visibility pop
#endif

#include "cJSON.h"

#ifdef true
#undef true
#endif
#define true ((cJSON_bool)1)

#ifdef false
#undef false
#endif
#define false ((cJSON_bool)0)

#ifndef isinf
#define isinf(d) (isnan((d) - (d)) && !isnan(d))
#endif
#ifndef isnan
#define isnan(d) ((d) != (d))
#endif

#ifndef NAN
#ifdef _WIN32
#define NAN sqrt(-1.0)
#else
#define NAN (0.0 / 0.0)
#endif
#endif

typedef struct {
    const unsigned char *json;
    size_t position;
} error;

static error global_error = { 0 };

CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void)
{
    return (const char *)(global_error.json + global_error.position);
}

CJSON_PUBLIC(char *) cJSON_GetStringValue(const cJSON * const item)
{
    if (!cJSON_IsString(item))
    {
        return NULL;
    }

    return item->valuestring;
}

CJSON_PUBLIC(double) cJSON_GetNumberValue(const cJSON * const item)
{
    if (!cJSON_IsNumber(item))
    {
        return NAN;
    }

    return item->valuedouble;
}

/* This is a safeguard to prevent copy-pasters from using incompatible C and header files */
#if (CJSON_VERSION_MAJOR != 1) || (CJSON_VERSION_MINOR != 7) || (CJSON_VERSION_PATCH != 19)
#error cJSON.h and cJSON.c have different versions. Make sure that both have the same.
#endif

CJSON_PUBLIC(const char*) cJSON_Version(void)
{
    static char version[15];
    snprintf(version, sizeof(version), "%i.%i.%i",
             CJSON_VERSION_MAJOR, CJSON_VERSION_MINOR, CJSON_VERSION_PATCH);

    return version;
}

/* Case insensitive string comparison, doesn't consider two NULL pointers equal though */
static int case_insensitive_strcmp(const unsigned char *string1, const unsigned char *string2)
{
    if (!string1 || !string2)
    {
        return 1;
    }

    if (string1 == string2)
    {
        return 0;
    }

    for (; tolower(*string1) == tolower(*string2); string1++, string2++)
    {
        if (*string1 == '\0')
        {
            return 0;
        }
    }

    return tolower(*string1) - tolower(*string2);
}

typedef struct internal_hooks {
    void *(CJSON_CDECL *allocate)(size_t size);
    void (CJSON_CDECL *deallocate)(void *pointer);
    void *(CJSON_CDECL *reallocate)(void *pointer, size_t size);
} internal_hooks;

#ifdef _MSC_VER
/* work around MSVC error C2322: '...' address of dllimport '...' is not static */
static void *CJSON_CDECL internal_malloc(size_t size)
{
    return malloc(size);
}

static void CJSON_CDECL internal_free(void *pointer)
{
    free(pointer);
}

static void *CJSON_CDECL internal_realloc(void *pointer, size_t size)
{
    return realloc(pointer, size);
}
#else
#define internal_malloc malloc
#define internal_free free
#define internal_realloc realloc
#endif

/* strlen of character literals resolved at compile time */
#define static_strlen(string_literal) (sizeof(string_literal) - sizeof(""))

static internal_hooks global_hooks = { internal_malloc, internal_free, internal_realloc };

static unsigned char *cJSON_strdup(const unsigned char *string, const internal_hooks * const hooks)
{
    size_t length = 0;
    unsigned char *copy = NULL;

    if (!string)
    {
        return NULL;
    }

    length = strlen((const char *)string) + sizeof("");
    copy = (unsigned char *)hooks->allocate(length);
    if (!copy)
    {
        return NULL;
    }
    memcpy(copy, string, length);

    return copy;
}

CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks *hooks)
{
    if (!hooks)
    {
        global_hooks.allocate = malloc;
        global_hooks.deallocate = free;
        global_hooks.reallocate = realloc;
        return;
    }

    global_hooks.allocate = malloc;
    if (hooks->malloc_fn)
    {
        global_hooks.allocate = hooks->malloc_fn;
    }

    global_hooks.deallocate = free;
    if (hooks->free_fn)
    {
        global_hooks.deallocate = hooks->free_fn;
    }

    global_hooks.reallocate = NULL;
    if ((global_hooks.allocate == malloc) && (global_hooks.deallocate == free))
    {
        global_hooks.reallocate = realloc;
    }
}

/* Internal constructor. */
static cJSON *cJSON_New_Item(const internal_hooks * const hooks)
{
    cJSON *node = (cJSON *)hooks->allocate(sizeof(cJSON));
    if (node)
    {
        memset(node, 0, sizeof(cJSON));
    }

    return node;
}

/* Delete a cJSON structure. */
CJSON_PUBLIC(void) cJSON_Delete(cJSON *item)
{
    cJSON *next = NULL;
    while (item)
    {
        next = item->next;
        if (!(item->type & cJSON_IsReference) && item->child)
        {
            cJSON_Delete(item->child);
        }
        if (!(item->type & cJSON_IsReference) && item->valuestring)
        {
            global_hooks.deallocate(item->valuestring);
            item->valuestring = NULL;
        }
        if (!(item->type & cJSON_StringIsConst) && item->string)
        {
            global_hooks.deallocate(item->string);
            item->string = NULL;
        }
        global_hooks.deallocate(item);
        item = next;
    }
}

/* get the decimal point character of the current locale */
static unsigned char get_decimal_point(void)
{
#ifdef ENABLE_LOCALES
    struct lconv *lconv = localeconv();
    return (unsigned char)lconv->decimal_point[0];
#else
    (void)0;
    return '.';
#endif
}

typedef struct {
    const unsigned char *content;
    size_t length;
    size_t offset;
    size_t depth;
    internal_hooks hooks;
} parse_buffer;

/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define can_read(buffer, size) ((buffer) && (((buffer)->offset + (size)) <= (buffer)->length))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define can_access_at_index(buffer, index) ((buffer) && (((buffer)->offset + (index)) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
/* get a pointer to the buffer at the position */
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)

/* Parse the input text to generate a number, and populate the result into item. */
static cJSON_bool parse_number(cJSON * const item, parse_buffer * const input_buffer);

static size_t parse_number_count_digits(const parse_buffer * const input_buffer, cJSON_bool *has_decimal)
{
    size_t count = 0;
    *has_decimal = false;

    while (can_access_at_index(input_buffer, count))
    {
        switch (buffer_at_offset(input_buffer)[count])
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
            case '-':
            case 'e':
            case 'E':
                count++;
                break;

            case '.':
                count++;
                *has_decimal = true;
                break;

            default:
                return count;
        }
    }
    return count;
}

static unsigned char *parse_number_copy_to_buffer(const parse_buffer * const input_buffer,
                                                   size_t length, unsigned char decimal_point)
{
    unsigned char *buffer = (unsigned char *)input_buffer->hooks.allocate(length + 1);
    if (!buffer)
    {
        return NULL;
    }

    memcpy(buffer, buffer_at_offset(input_buffer), length);
    buffer[length] = '\0';

    for (size_t i = 0; i < length; i++)
    {
        if (buffer[i] == '.')
        {
            buffer[i] = decimal_point;
        }
    }

    return buffer;
}

static void parse_number_set_item(cJSON * const item, double number)
{
    item->valuedouble = number;

    if (number >= INT_MAX)
    {
        item->valueint = INT_MAX;
    }
    else if (number <= (double)INT_MIN)
    {
        item->valueint = INT_MIN;
    }
    else
    {
        item->valueint = (int)number;
    }

    item->type = cJSON_Number;
}

static cJSON_bool parse_number(cJSON * const item, parse_buffer * const input_buffer)
{
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char *number_c_string = NULL;
    unsigned char decimal_point = get_decimal_point();
    size_t number_string_length = 0;
    cJSON_bool has_decimal_point = false;

    if (!input_buffer || !input_buffer->content)
    {
        return false;
    }

    number_string_length = parse_number_count_digits(input_buffer, &has_decimal_point);
    if (number_string_length == 0)
    {
        return false;
    }

    number_c_string = parse_number_copy_to_buffer(input_buffer, number_string_length, decimal_point);
    if (!number_c_string)
    {
        return false;
    }

    number = strtod((const char *)number_c_string, (char **)&after_end);
    if (number_c_string == after_end)
    {
        input_buffer->hooks.deallocate(number_c_string);
        return false;
    }

    parse_number_set_item(item, number);

    input_buffer->offset += (size_t)(after_end - number_c_string);
    input_buffer->hooks.deallocate(number_c_string);
    return true;
}

/* don't ask me, but the original cJSON_SetNumberValue returns an integer or double */
CJSON_PUBLIC(double) cJSON_SetNumberHelper(cJSON *object, double number)
{
    if (!object)
    {
        return NAN;
    }

    if (number >= INT_MAX)
    {
        object->valueint = INT_MAX;
    }
    else if (number <= (double)INT_MIN)
    {
        object->valueint = INT_MIN;
    }
    else
    {
        object->valueint = (int)number;
    }

    return object->valuedouble = number;
}

/* Note: when passing a NULL valuestring, cJSON_SetValuestring treats this as an error and return NULL */
CJSON_PUBLIC(char *) cJSON_SetValuestring(cJSON *object, const char *valuestring)
{
    char *copy = NULL;
    size_t v1_len;
    size_t v2_len;

    if (!object || !(object->type & cJSON_String) || (object->type & cJSON_IsReference))
    {
        return NULL;
    }
    if (!object->valuestring || !valuestring)
    {
        return NULL;
    }

    v1_len = strlen(valuestring);
    v2_len = strlen(object->valuestring);

    if (v1_len <= v2_len)
    {
        if (!(valuestring + v1_len < object->valuestring || object->valuestring + v2_len < valuestring))
        {
            return NULL;
        }
        strcpy(object->valuestring, valuestring);
        return object->valuestring;
    }
    copy = (char *)cJSON_strdup((const unsigned char *)valuestring, &global_hooks);
    if (!copy)
    {
        return NULL;
    }
    if (object->valuestring)
    {
        cJSON_free(object->valuestring);
    }
    object->valuestring = copy;

    return copy;
}

typedef struct {
    unsigned char *buffer;
    size_t length;
    size_t offset;
    size_t depth;
    cJSON_bool noalloc;
    cJSON_bool format;
    internal_hooks hooks;
} printbuffer;

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static unsigned char *ensure(printbuffer * const p, size_t needed)
{
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;

    if (!p || !p->buffer)
    {
        return NULL;
    }

    if (p->length > 0 && p->offset >= p->length)
    {
        return NULL;
    }

    if (needed > INT_MAX)
    {
        return NULL;
    }

    needed += p->offset + 1;
    if (needed <= p->length)
    {
        return p->buffer + p->offset;
    }

    if (p->noalloc)
    {
        return NULL;
    }

    if (needed > (INT_MAX / 2))
    {
        if (needed <= INT_MAX)
        {
            newsize = INT_MAX;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        newsize = needed * 2;
    }

    if (p->hooks.reallocate)
    {
        newbuffer = (unsigned char *)p->hooks.reallocate(p->buffer, newsize);
        if (!newbuffer)
        {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;

            return NULL;
        }
    }
    else
    {
        newbuffer = (unsigned char *)p->hooks.allocate(newsize);
        if (!newbuffer)
        {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;

            return NULL;
        }

        memcpy(newbuffer, p->buffer, p->offset + 1);
        p->hooks.deallocate(p->buffer);
    }
    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

/* calculate the new length of the string in a printbuffer and update the offset */
static void update_offset(printbuffer * const buffer)
{
    const unsigned char *buffer_pointer = NULL;
    if (!buffer || !buffer->buffer)
    {
        return;
    }
    buffer_pointer = buffer->buffer + buffer->offset;

    buffer->offset += strlen((const char *)buffer_pointer);
}

/* securely comparison of floating-point variables */
static inline cJSON_bool compare_double(double a, double b)
{
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= maxVal * DBL_EPSILON);
}

/* Render the number nicely from the given item into a string. */
static cJSON_bool print_number(const cJSON * const item, printbuffer * const output_buffer);

static int print_number_format_buffer(double d, int valueint, unsigned char *number_buffer, size_t buffer_size)
{
    double test = 0.0;
    int length = 0;

    if (isnan(d) || isinf(d))
    {
        return snprintf((char *)number_buffer, buffer_size, "null");
    }

    if (d == (double)valueint)
    {
        return snprintf((char *)number_buffer, buffer_size, "%d", valueint);
    }

    length = snprintf((char *)number_buffer, buffer_size, "%1.15g", d);

    if ((sscanf((char *)number_buffer, "%lg", &test) != 1) || !compare_double(test, d))
    {
        length = snprintf((char *)number_buffer, buffer_size, "%1.17g", d);
    }

    return length;
}

static cJSON_bool print_number(const cJSON * const item, printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    double d = item->valuedouble;
    int length = 0;
    size_t i = 0;
    unsigned char number_buffer[26] = { 0 };
    unsigned char decimal_point = get_decimal_point();

    if (!output_buffer)
    {
        return false;
    }

    length = print_number_format_buffer(d, item->valueint, number_buffer, sizeof(number_buffer));

    if (length < 0 || (size_t)length >= sizeof(number_buffer))
    {
        return false;
    }

    output_pointer = ensure(output_buffer, (size_t)length + sizeof(""));
    if (!output_pointer)
    {
        return false;
    }

    for (i = 0; i < (size_t)length; i++)
    {
        if (number_buffer[i] == decimal_point)
        {
            output_pointer[i] = '.';
            continue;
        }

        output_pointer[i] = number_buffer[i];
    }
    output_pointer[i] = '\0';

    output_buffer->offset += (size_t)length;

    return true;
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const unsigned char * const input)
{
    unsigned int h = 0;
    size_t i = 0;

    for (i = 0; i < 4; i++)
    {
        if (input[i] >= '0' && input[i] <= '9')
        {
            h += (unsigned int)input[i] - '0';
        }
        else if (input[i] >= 'A' && input[i] <= 'F')
        {
            h += (unsigned int)10 + input[i] - 'A';
        }
        else if (input[i] >= 'a' && input[i] <= 'f')
        {
            h += (unsigned int)10 + input[i] - 'a';
        }
        else
        {
            return 0;
        }

        if (i < 3)
        {
            h = h << 4;
        }
    }

    return h;
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static unsigned char utf16_literal_to_utf8(const unsigned char * const input_pointer,
                                          const unsigned char * const input_end,
                                          unsigned char **output_pointer)
{
    long unsigned int codepoint = 0;
    unsigned int first_code = 0;
    const unsigned char *first_sequence = input_pointer;
    unsigned char utf8_length = 0;
    unsigned char utf8_position = 0;
    unsigned char sequence_length = 0;
    unsigned char first_byte_mark = 0;

    if ((input_end - first_sequence) < 6)
    {
        goto fail;
    }

    first_code = parse_hex4(first_sequence + 2);

    if ((first_code >= 0xDC00) && (first_code <= 0xDFFF))
    {
        goto fail;
    }

    if (first_code >= 0xD800 && first_code <= 0xDBFF)
    {
        const unsigned char *second_sequence = first_sequence + 6;
        unsigned int second_code = 0;
        sequence_length = 12;

        if ((input_end - second_sequence) < 6)
        {
            goto fail;
        }

        if (second_sequence[0] != '\\' || second_sequence[1] != 'u')
        {
            goto fail;
        }

        second_code = parse_hex4(second_sequence + 2);
        if (second_code < 0xDC00 || second_code > 0xDFFF)
        {
            goto fail;
        }

        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    }
    else
    {
        sequence_length = 6;
        codepoint = first_code;
    }

    if (codepoint < 0x80)
    {
        utf8_length = 1;
    }
    else if (codepoint < 0x800)
    {
        utf8_length = 2;
        first_byte_mark = 0xC0;
    }
    else if (codepoint < 0x10000)
    {
        utf8_length = 3;
        first_byte_mark = 0xE0;
    }
    else if (codepoint <= 0x10FFFF)
    {
        utf8_length = 4;
        first_byte_mark = 0xF0;
    }
    else
    {
        goto fail;
    }

    for (utf8_position = (unsigned char)(utf8_length - 1); utf8_position > 0; utf8_position--)
    {
        (*output_pointer)[utf8_position] = (unsigned char)((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    if (utf8_length > 1)
    {
        (*output_pointer)[0] = (unsigned char)((codepoint | first_byte_mark) & 0xFF);
    }
    else
    {
        (*output_pointer)[0] = (unsigned char)(codepoint & 0x7F);
    }

    *output_pointer += utf8_length;

    return sequence_length;

fail:
    return 0;
}

/* Parse the input text into an unescaped cinput, and populate item. */
static cJSON_bool parse_string(cJSON * const item, parse_buffer * const input_buffer);

static const unsigned char *parse_string_find_end(const parse_buffer * const input_buffer,
                                                  const unsigned char *input_end,
                                                  size_t *skipped_bytes)
{
    *skipped_bytes = 0;

    while (((size_t)(input_end - input_buffer->content) < input_buffer->length) && *input_end != '\"')
    {
        if (input_end[0] == '\\')
        {
            if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length)
            {
                return NULL;
            }
            (*skipped_bytes)++;
            input_end++;
        }
        input_end++;
    }

    if (((size_t)(input_end - input_buffer->content) >= input_buffer->length) || (*input_end != '\"'))
    {
        return NULL;
    }

    return input_end;
}

static unsigned char *parse_string_allocate_buffer(const parse_buffer * const input_buffer,
                                                   const unsigned char *input_end,
                                                   size_t skipped_bytes)
{
    size_t allocation_length = (size_t)(input_end - buffer_at_offset(input_buffer)) - skipped_bytes;
    return (unsigned char *)input_buffer->hooks.allocate(allocation_length + sizeof(""));
}

static unsigned char parse_string_decode_escape(const unsigned char **input_pointer,
                                                 const unsigned char *input_end,
                                                 unsigned char **output_pointer)
{
    unsigned char sequence_length = 2;

    if ((input_end - *input_pointer) < 1)
    {
        return 0;
    }

    switch ((*input_pointer)[1])
    {
        case 'b':
            *(*output_pointer)++ = '\b';
            break;
        case 'n':
            *(*output_pointer)++ = '\n';
            break;
        case 'r':
            *(*output_pointer)++ = '\r';
            break;
        case 't':
            *(*output_pointer)++ = '\t';
            break;
        case '\"':
        case '\\':
        case '/':
            *(*output_pointer)++ = (*input_pointer)[1];
            break;

        case 'u':
            sequence_length = utf16_literal_to_utf8(*input_pointer, input_end, output_pointer);
            if (sequence_length == 0)
            {
                return 0;
            }
            break;

        default:
            return 0;
    }

    return sequence_length;
}

static cJSON_bool parse_string_copy_content(const unsigned char *input_pointer,
                                             const unsigned char *input_end,
                                             unsigned char *output)
{
    unsigned char *output_pointer = output;

    while (input_pointer < input_end)
    {
        if (*input_pointer != '\\')
        {
            *output_pointer++ = *input_pointer++;
        }
        else
        {
            unsigned char seq_len = parse_string_decode_escape(&input_pointer, input_end, &output_pointer);
            if (seq_len == 0)
            {
                return false;
            }
            input_pointer += seq_len;
        }
    }

    *output_pointer = '\0';
    return true;
}

static cJSON_bool parse_string(cJSON * const item, parse_buffer * const input_buffer)
{
    const unsigned char *input_pointer = buffer_at_offset(input_buffer) + 1;
    const unsigned char *input_end = buffer_at_offset(input_buffer) + 1;
    unsigned char *output = NULL;
    size_t skipped_bytes = 0;

    if (buffer_at_offset(input_buffer)[0] != '\"')
    {
        goto fail;
    }

    output = parse_string_allocate_buffer(input_buffer, input_end, skipped_bytes);
    if (!output)
    {
        goto fail;
    }

    if (!parse_string_copy_content(input_pointer, input_end, output))
    {
        input_buffer->hooks.deallocate(output);
        goto fail;
    }

    item->type = cJSON_String;
    item->valuestring = (char *)output;

    input_buffer->offset = (size_t)(input_end - input_buffer->content);
    input_buffer->offset++;

    return true;

fail:
    if (output)
    {
        input_buffer->hooks.deallocate(output);
    }

    if (input_pointer)
    {
        input_buffer->offset = (size_t)(input_pointer - input_buffer->content);
    }

    return false;
}

/* Render the cstring provided to an escaped version that can be printed. */
static cJSON_bool print_string_ptr(const unsigned char * const input, printbuffer * const output_buffer);

static size_t print_string_count_escapes(const unsigned char *input)
{
    size_t escape_characters = 0;
    const unsigned char *input_pointer = input;

    for (input_pointer = input; *input_pointer; input_pointer++)
    {
        switch (*input_pointer)
        {
            case '\"':
            case '\\':
            case '\b':
            case '\n':
            case '\r':
            case '\t':
                escape_characters++;
                break;
            default:
                if (*input_pointer < 32)
                {
                    escape_characters += 5;
                }
                break;
        }
    }

    return escape_characters;
}

static unsigned char print_string_get_escape_char(unsigned char c)
{
    switch (c)
    {
        case '\\':
            return '\\';
        case '\"':
            return '\"';
        case '\b':
            return 'b';
        case '\n':
            return 'n';
        case '\r':
            return 'r';
        case '\t':
            return 't';
        default:
            return 0;
    }
}

static void print_string_copy_with_escapes(const unsigned char *input, unsigned char *output,
                                           size_t input_length)
{
    const unsigned char *input_pointer = input;
    unsigned char *output_pointer = output + 1;

    for (size_t i = 0; i < input_length && *input_pointer != '\0'; i++, input_pointer++)
    {
        unsigned char escape_char = print_string_get_escape_char(*input_pointer);
        if (escape_char)
        {
            *output_pointer = escape_char;
        }
        else if (*input_pointer > 31 && *input_pointer != '\"' && *input_pointer != '\\')
        {
            *output_pointer = *input_pointer;
        }
        else
        {
            *output_pointer++ = '\\';
            snprintf((char *)output_pointer, 6, "u%04x", *input_pointer);
            output_pointer += 4;
            continue;
        }
        output_pointer++;
    }
}

static cJSON_bool print_string_ptr(const unsigned char * const input, printbuffer * const output_buffer)
{
    unsigned char *output = NULL;
    size_t output_length = 0;
    size_t escape_characters = 0;

    if (!output_buffer)
    {
        return false;
    }

    if (!input)
    {
        output = ensure(output_buffer, sizeof("\"\""));
        if (!output)
        {
            return false;
        }
        strcpy((char *)output, "\"\"");

        return true;
    }

    escape_characters = print_string_count_escapes(input);
    output_length = strlen((const char *)input) + escape_characters;

    output = ensure(output_buffer, output_length + sizeof("\"\""));
    if (!output)
    {
        return false;
    }

    if (escape_characters == 0)
    {
        output[0] = '\"';
        memcpy(output + 1, input, output_length);
        output[output_length + 1] = '\"';
        output[output_length + 2] = '\0';

        return true;
    }

    output[0] = '\"';
    print_string_copy_with_escapes(input, output, strlen((const char *)input));
    output[output_length + 1] = '\"';
    output[output_length + 2] = '\0';

    return true;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static cJSON_bool print_string(const cJSON * const item, printbuffer * const p)
{
    return print_string_ptr((unsigned char *)item->valuestring, p);
}

/* Predeclare these prototypes. */
static cJSON_bool parse_value(cJSON * const item, parse_buffer * const input_buffer);
static cJSON_bool print_value(const cJSON * const item, printbuffer * const output_buffer);
static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer);
static cJSON_bool print_array(const cJSON * const item, printbuffer * const output_buffer);
static cJSON_bool parse_object(cJSON * const item, parse_buffer * const input_buffer);
static cJSON_bool print_object(const cJSON * const item, printbuffer * const output_buffer);

/* Utility to jump whitespace and cr/lf */
static parse_buffer *buffer_skip_whitespace(parse_buffer * const buffer)
{
    if (!buffer || !buffer->content)
    {
        return NULL;
    }

    if (cannot_access_at_index(buffer, 0))
    {
        return buffer;
    }

    while (can_access_at_index(buffer, 0) && (buffer_at_offset(buffer)[0] <= 32))
    {
        buffer->offset++;
    }

    if (buffer->offset == buffer->length)
    {
        buffer->offset--;
    }

    return buffer;
}

/* skip the UTF-8 BOM (byte order mark) if it is at the beginning of a buffer */
static parse_buffer *skip_utf8_bom(parse_buffer * const buffer)
{
    if (!buffer || !buffer->content || buffer->offset != 0)
    {
        return NULL;
    }

    if (can_access_at_index(buffer, 4) && strncmp((const char *)buffer_at_offset(buffer), "\xEF\xBB\xBF", 3) == 0)
    {
        buffer->offset += 3;
    }

    return buffer;
}

CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated)
{
    size_t buffer_length;

    if (!value)
    {
        return NULL;
    }

    buffer_length = strlen(value) + sizeof("");

    return cJSON_ParseWithLengthOpts(value, buffer_length, return_parse_end, require_null_terminated);
}

/* Parse an object - create a new root, and populate. */
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length,
                                                const char **return_parse_end,
                                                cJSON_bool require_null_terminated)
{
    parse_buffer buffer = { 0 };
    cJSON *item = NULL;

    global_error.json = NULL;
    global_error.position = 0;

    if (!value || buffer_length == 0)
    {
        goto fail;
    }

    buffer.content = (const unsigned char *)value;
    buffer.length = buffer_length;
    buffer.offset = 0;
    buffer.hooks = global_hooks;

    item = cJSON_New_Item(&global_hooks);
    if (!item)
    {
        goto fail;
    }

    if (!parse_value(item, buffer_skip_whitespace(skip_utf8_bom(&buffer))))
    {
        goto fail;
    }

    if (require_null_terminated)
    {
        buffer_skip_whitespace(&buffer);
        if (buffer.offset >= buffer.length || buffer_at_offset(&buffer)[0] != '\0')
        {
            goto fail;
        }
    }
    if (return_parse_end)
    {
        *return_parse_end = (const char *)buffer_at_offset(&buffer);
    }

    return item;

fail:
    if (item)
    {
        cJSON_Delete(item);
    }

    if (value)
    {
        error local_error = { 0 };
        local_error.json = (const unsigned char *)value;
        local_error.position = 0;

        if (buffer.offset < buffer.length)
        {
            local_error.position = buffer.offset;
        }
        else if (buffer.length > 0)
        {
            local_error.position = buffer.length - 1;
        }

        if (return_parse_end)
        {
            *return_parse_end = (const char *)local_error.json + local_error.position;
        }

        global_error = local_error;
    }

    return NULL;
}

/* Default options for cJSON_Parse */
CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value)
{
    return cJSON_ParseWithOpts(value, NULL, 0);
}

CJSON_PUBLIC(cJSON *) cJSON_ParseWithLength(const char *value, size_t buffer_length)
{
    return cJSON_ParseWithLengthOpts(value, buffer_length, NULL, 0);
}

#define cjson_min(a, b) (((a) < (b)) ? (a) : (b))

static unsigned char *print(const cJSON * const item, cJSON_bool format, const internal_hooks * const hooks)
{
    static const size_t default_buffer_size = 256;
    printbuffer buffer = { 0 };
    unsigned char *printed = NULL;

    buffer.buffer = (unsigned char *)hooks->allocate(default_buffer_size);
    buffer.length = default_buffer_size;
    buffer.format = format;
    buffer.hooks = *hooks;
    if (!buffer.buffer)
    {
        goto fail;
    }

    if (!print_value(item, &buffer))
    {
        goto fail;
    }
    update_offset(&buffer);

    if (hooks->reallocate)
    {
        printed = (unsigned char *)hooks->reallocate(buffer.buffer, buffer.offset + 1);
        if (!printed)
        {
            goto fail;
        }
        buffer.buffer = NULL;
    }
    else
    {
        printed = (unsigned char *)hooks->allocate(buffer.offset + 1);
        if (!printed)
        {
            goto fail;
        }
        memcpy(printed, buffer.buffer, cjson_min(buffer.length, buffer.offset + 1));
        printed[buffer.offset] = '\0';

        hooks->deallocate(buffer.buffer);
        buffer.buffer = NULL;
    }

    return printed;

fail:
    if (buffer.buffer)
    {
        hooks->deallocate(buffer.buffer);
        buffer.buffer = NULL;
    }

    if (printed)
    {
        hooks->deallocate(printed);
        printed = NULL;
    }

    return NULL;
}

/* Render a cJSON item/entity/structure to text. */
CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item)
{
    return (char *)print(item, true, &global_hooks);
}

CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item)
{
    return (char *)print(item, false, &global_hooks);
}

CJSON_PUBLIC(char *) cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt)
{
    printbuffer p = { 0 };

    if (prebuffer < 0)
    {
        return NULL;
    }

    p.buffer = (unsigned char *)global_hooks.allocate((size_t)prebuffer);
    if (!p.buffer)
    {
        return NULL;
    }

    p.length = (size_t)prebuffer;
    p.offset = 0;
    p.noalloc = false;
    p.format = fmt;
    p.hooks = global_hooks;

    if (!print_value(item, &p))
    {
        global_hooks.deallocate(p.buffer);
        p.buffer = NULL;
        return NULL;
    }

    return (char *)p.buffer;
}

CJSON_PUBLIC(cJSON_bool) cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format)
{
    printbuffer p = { 0 };

    if (length < 0 || !buffer)
    {
        return false;
    }

    p.buffer = (unsigned char *)buffer;
    p.length = (size_t)length;
    p.offset = 0;
    p.noalloc = true;
    p.format = format;
    p.hooks = global_hooks;

    return print_value(item, &p);
}

/* Parser core - when encountering text, process appropriately. */
static cJSON_bool parse_value(cJSON * const item, parse_buffer * const input_buffer)
{
    if (!input_buffer || !input_buffer->content)
    {
        return false;
    }

    if (can_read(input_buffer, 4) && strncmp((const char *)buffer_at_offset(input_buffer), "null", 4) == 0)
    {
        item->type = cJSON_NULL;
        input_buffer->offset += 4;
        return true;
    }
    if (can_read(input_buffer, 5) && strncmp((const char *)buffer_at_offset(input_buffer), "false", 5) == 0)
    {
        item->type = cJSON_False;
        input_buffer->offset += 5;
        return true;
    }
    if (can_read(input_buffer, 4) && strncmp((const char *)buffer_at_offset(input_buffer), "true", 4) == 0)
    {
        item->type = cJSON_True;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == '\"')
    {
        return parse_string(item, input_buffer);
    }
    if (can_access_at_index(input_buffer, 0) &&
        (buffer_at_offset(input_buffer)[0] == '-' ||
         (buffer_at_offset(input_buffer)[0] >= '0' && buffer_at_offset(input_buffer)[0] <= '9')))
    {
        return parse_number(item, input_buffer);
    }
    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == '[')
    {
        return parse_array(item, input_buffer);
    }
    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == '{')
    {
        return parse_object(item, input_buffer);
    }

    return false;
}

/* Render a value to text. */
static cJSON_bool print_value(const cJSON * const item, printbuffer * const output_buffer)
{
    unsigned char *output = NULL;

    if (!item || !output_buffer)
    {
        return false;
    }

    switch (item->type & 0xFF)
    {
        case cJSON_NULL:
            output = ensure(output_buffer, 5);
            if (!output)
            {
                return false;
            }
            strcpy((char *)output, "null");
            return true;

        case cJSON_False:
            output = ensure(output_buffer, 6);
            if (!output)
            {
                return false;
            }
            strcpy((char *)output, "false");
            return true;

        case cJSON_True:
            output = ensure(output_buffer, 5);
            if (!output)
            {
                return false;
            }
            strcpy((char *)output, "true");
            return true;

        case cJSON_Number:
            return print_number(item, output_buffer);

        case cJSON_Raw:
        {
            size_t raw_length = 0;
            if (!item->valuestring)
            {
                return false;
            }

            raw_length = strlen(item->valuestring) + sizeof("");
            output = ensure(output_buffer, raw_length);
            if (!output)
            {
                return false;
            }
            memcpy(output, item->valuestring, raw_length);
            return true;
        }

        case cJSON_String:
            return print_string(item, output_buffer);

        case cJSON_Array:
            return print_array(item, output_buffer);

        case cJSON_Object:
            return print_object(item, output_buffer);

        default:
            return false;
    }
}

/* Build an array from input text. */
static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer);

static cJSON_bool parse_array_append_item(cJSON **head, cJSON **current_item, cJSON *new_item)
{
    if (!new_item)
    {
        return false;
    }

    if (!*head)
    {
        *current_item = *head = new_item;
    }
    else
    {
        (*current_item)->next = new_item;
        new_item->prev = *current_item;
        *current_item = new_item;
    }

    return true;
}

static cJSON_bool parse_array_parse_elements(cJSON **head, cJSON **current_item,
                                              parse_buffer * const input_buffer)
{
    do
    {
        cJSON *new_item = cJSON_New_Item(&input_buffer->hooks);
        if (!new_item)
        {
            return false;
        }

        if (!parse_array_append_item(head, current_item, new_item))
        {
            input_buffer->hooks.deallocate(new_item);
            return false;
        }

        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(*current_item, input_buffer))
        {
            return false;
        }
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == ',');

    return true;
}

static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer)
{
    cJSON *head = NULL;
    cJSON *current_item = NULL;

    if (input_buffer->depth >= CJSON_NESTING_LIMIT)
    {
        return false;
    }
    input_buffer->depth++;

    if (buffer_at_offset(input_buffer)[0] != '[')
    {
        goto fail;
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == ']')
    {
        goto success;
    }

    if (cannot_access_at_index(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    input_buffer->offset--;
    if (!parse_array_parse_elements(&head, &current_item, input_buffer))
    {
        goto fail;
    }

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ']')
    {
        goto fail;
    }

success:
    input_buffer->depth--;

    if (head)
    {
        head->prev = current_item;
    }

    item->type = cJSON_Array;
    item->child = head;

    input_buffer->offset++;

    return true;

fail:
    if (head)
    {
        cJSON_Delete(head);
    }

    return false;
}

/* Render an array to text */
static cJSON_bool print_array(const cJSON * const item, printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    cJSON *current_element = item->child;

    if (!output_buffer)
    {
        return false;
    }

    if (output_buffer->depth >= CJSON_NESTING_LIMIT)
    {
        return false;
    }

    output_pointer = ensure(output_buffer, 1);
    if (!output_pointer)
    {
        return false;
    }

    *output_pointer = '[';
    output_buffer->offset++;
    output_buffer->depth++;

    while (current_element)
    {
        if (!print_value(current_element, output_buffer))
        {
            return false;
        }
        update_offset(output_buffer);
        if (current_element->next)
        {
            length = output_buffer->format ? 2 : 1;
            output_pointer = ensure(output_buffer, length + 1);
            if (!output_pointer)
            {
                return false;
            }
            *output_pointer++ = ',';
            if (output_buffer->format)
            {
                *output_pointer++ = ' ';
            }
            *output_pointer = '\0';
            output_buffer->offset += length;
        }
        current_element = current_element->next;
    }

    output_pointer = ensure(output_buffer, 2);
    if (!output_pointer)
    {
        return false;
    }
    *output_pointer++ = ']';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Build an object from the text. */
static cJSON_bool parse_object(cJSON * const item, parse_buffer * const input_buffer);

static cJSON_bool parse_object_append_item(cJSON **head, cJSON **current_item, cJSON *new_item)
{
    if (!new_item)
    {
        return false;
    }

    if (!*head)
    {
        *current_item = *head = new_item;
    }
    else
    {
        (*current_item)->next = new_item;
        new_item->prev = *current_item;
        *current_item = new_item;
    }

    return true;
}

static cJSON_bool parse_object_parse_members(cJSON **head, cJSON **current_item,
                                              parse_buffer * const input_buffer)
{
    do
    {
        cJSON *new_item = cJSON_New_Item(&input_buffer->hooks);
        if (!new_item)
        {
            return false;
        }

        if (!parse_object_append_item(head, current_item, new_item))
        {
            input_buffer->hooks.deallocate(new_item);
            return false;
        }

        if (cannot_access_at_index(input_buffer, 0))
        {
            return false;
        }

        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_string(*current_item, input_buffer))
        {
            return false;
        }
        buffer_skip_whitespace(input_buffer);

        (*current_item)->string = (*current_item)->valuestring;
        (*current_item)->valuestring = NULL;

        if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ':')
        {
            return false;
        }

        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(*current_item, input_buffer))
        {
            return false;
        }
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == ',');

    return true;
}

static cJSON_bool parse_object(cJSON * const item, parse_buffer * const input_buffer)
{
    cJSON *head = NULL;
    cJSON *current_item = NULL;

    if (input_buffer->depth >= CJSON_NESTING_LIMIT)
    {
        return false;
    }
    input_buffer->depth++;

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != '{')
    {
        goto fail;
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == '}')
    {
        goto success;
    }

    if (cannot_access_at_index(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    input_buffer->offset--;
    if (!parse_object_parse_members(&head, &current_item, input_buffer))
    {
        goto fail;
    }

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != '}')
    {
        goto fail;
    }

success:
    input_buffer->depth--;

    if (head)
    {
        head->prev = current_item;
    }

    item->type = cJSON_Object;
    item->child = head;

    input_buffer->offset++;
    return true;

fail:
    if (head)
    {
        cJSON_Delete(head);
    }

    return false;
}

/* Render an object to text. */
static cJSON_bool print_object(const cJSON * const item, printbuffer * const output_buffer);

static cJSON_bool print_object_open_brace(printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = output_buffer->format ? 2 : 1;

    output_pointer = ensure(output_buffer, length + 1);
    if (!output_pointer)
    {
        return false;
    }

    *output_pointer++ = '{';
    output_buffer->depth++;
    if (output_buffer->format)
    {
        *output_pointer++ = '\n';
    }
    output_buffer->offset += length;

    return true;
}

static cJSON_bool print_object_print_indent(printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t i = 0;

    output_pointer = ensure(output_buffer, output_buffer->depth);
    if (!output_pointer)
    {
        return false;
    }
    for (i = 0; i < output_buffer->depth; i++)
    {
        *output_pointer++ = '\t';
    }
    output_buffer->offset += output_buffer->depth;

    return true;
}

static cJSON_bool print_object_print_member(cJSON *current_item, printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;

    if (output_buffer->format)
    {
        if (!print_object_print_indent(output_buffer))
        {
            return false;
        }
    }

    if (!print_string_ptr((unsigned char *)current_item->string, output_buffer))
    {
        return false;
    }
    update_offset(output_buffer);

    length = output_buffer->format ? 2 : 1;
    output_pointer = ensure(output_buffer, length);
    if (!output_pointer)
    {
        return false;
    }
    *output_pointer++ = ':';
    if (output_buffer->format)
    {
        *output_pointer++ = '\t';
    }
    output_buffer->offset += length;

    if (!print_value(current_item, output_buffer))
    {
        return false;
    }
    update_offset(output_buffer);

    length = ((output_buffer->format ? 1 : 0) + (current_item->next ? 1 : 0));
    output_pointer = ensure(output_buffer, length + 1);
    if (!output_pointer)
    {
        return false;
    }
    if (current_item->next)
    {
        *output_pointer++ = ',';
    }

    if (output_buffer->format)
    {
        *output_pointer++ = '\n';
    }
    *output_pointer = '\0';
    output_buffer->offset += length;

    return true;
}

static cJSON_bool print_object_close_brace(printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t i = 0;

    output_pointer = ensure(output_buffer, output_buffer->format ? (output_buffer->depth + 1) : 2);
    if (!output_pointer)
    {
        return false;
    }
    if (output_buffer->format)
    {
        for (i = 0; i < output_buffer->depth - 1; i++)
        {
            *output_pointer++ = '\t';
        }
    }
    *output_pointer++ = '}';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

static cJSON_bool print_object(const cJSON * const item, printbuffer * const output_buffer)
{
    cJSON *current_item = item->child;

    if (!output_buffer)
    {
        return false;
    }

    if (output_buffer->depth >= CJSON_NESTING_LIMIT)
    {
        return false;
    }

    if (!print_object_open_brace(output_buffer))
    {
        return false;
    }

    while (current_item)
    {
        if (!print_object_print_member(current_item, output_buffer))
        {
            return false;
        }
        current_item = current_item->next;
    }

    if (!print_object_close_brace(output_buffer))
    {
        return false;
    }

    return true;
}

/* Get Array size/item / object item. */
CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array)
{
    cJSON *child = NULL;
    size_t size = 0;

    if (!array)
    {
        return 0;
    }

    child = array->child;

    while (child)
    {
        size++;
        child = child->next;
    }

    return (int)size;
}

static cJSON *get_array_item(const cJSON *array, size_t index)
{
    cJSON *current_child = NULL;

    if (!array)
    {
        return NULL;
    }

    current_child = array->child;
    while (current_child && index > 0)
    {
        index--;
        current_child = current_child->next;
    }

    return current_child;
}

CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index)
{
    if (index < 0)
    {
        return NULL;
    }

    return get_array_item(array, (size_t)index);
}

static cJSON *get_object_item(const cJSON * const object, const char * const name,
                               const cJSON_bool case_sensitive)
{
    cJSON *current_element = NULL;

    if (!object || !name)
    {
        return NULL;
    }

    current_element = object->child;
    if (case_sensitive)
    {
        while (current_element && current_element->string && strcmp(name, current_element->string) != 0)
        {
            current_element = current_element->next;
        }
    }
    else
    {
        while (current_element && case_insensitive_strcmp((const unsigned char *)name,
                                                          (const unsigned char *)(current_element->string)) != 0)
        {
            current_element = current_element->next;
        }
    }

    if (!current_element || !current_element->string)
    {
        return NULL;
    }

    return current_element;
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(const cJSON * const object, const char * const string)
{
    return get_object_item(object, string, false);
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string)
{
    return get_object_item(object, string, true);
}

CJSON_PUBLIC(cJSON_bool) cJSON_HasObjectItem(const cJSON *object, const char *string)
{
    return cJSON_GetObjectItem(object, string) ? 1 : 0;
}

/* Utility for array list handling. */
static void suffix_object(cJSON *prev, cJSON *item)
{
    prev->next = item;
    item->prev = prev;
}

/* Utility for handling references. */
static cJSON *create_reference(const cJSON *item, const internal_hooks * const hooks)
{
    cJSON *reference = NULL;
    if (!item)
    {
        return NULL;
    }

    reference = cJSON_New_Item(hooks);
    if (!reference)
    {
        return NULL;
    }

    memcpy(reference, item, sizeof(cJSON));
    reference->string = NULL;
    reference->type |= cJSON_IsReference;
    reference->next = reference->prev = NULL;
    return reference;
}

static cJSON_bool add_item_to_array(cJSON *array, cJSON *item)
{
    cJSON *child = NULL;

    if (!item || !array || array == item)
    {
        return false;
    }

    child = array->child;
    if (!child)
    {
        array->child = item;
        item->prev = item;
        item->next = NULL;
    }
    else
    {
        if (child->prev)
        {
            suffix_object(child->prev, item);
            array->child->prev = item;
        }
    }

    return true;
}

/* Add item to array/object. */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    return add_item_to_array(array, item);
}

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
/* helper function to cast away const */
static void *cast_away_const(const void *string)
{
    return (void *)string;
}
#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif


static cJSON_bool add_item_to_object(cJSON * const object, const char * const string,
                                      cJSON * const item, const internal_hooks * const hooks,
                                      const cJSON_bool constant_key)
{
    char *new_key = NULL;
    int new_type = cJSON_Invalid;

    if (!object || !string || !item || object == item)
    {
        return false;
    }

    if (constant_key)
    {
        new_key = (char *)cast_away_const(string);
        new_type = item->type | cJSON_StringIsConst;
    }
    else
    {
        new_key = (char *)cJSON_strdup((const unsigned char *)string, hooks);
        if (!new_key)
        {
            return false;
        }

        new_type = item->type & ~cJSON_StringIsConst;
    }

    if (!(item->type & cJSON_StringIsConst) && item->string)
    {
        hooks->deallocate(item->string);
    }

    item->string = new_key;
    item->type = new_type;

    return add_item_to_array(object, item);
}

CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    return add_item_to_object(object, string, item, &global_hooks, false);
}

/* Add an item to an object with constant string as key */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item)
{
    return add_item_to_object(object, string, item, &global_hooks, true);
}

CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)
{
    if (!array)
    {
        return false;
    }

    return add_item_to_array(array, create_reference(item, &global_hooks));
}

CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item)
{
    if (!object || !string)
    {
        return false;
    }

    return add_item_to_object(object, string, create_reference(item, &global_hooks), &global_hooks, false);
}

CJSON_PUBLIC(cJSON *) cJSON_AddNullToObject(cJSON * const object, const char * const name)
{
    cJSON *null = cJSON_CreateNull();
    if (add_item_to_object(object, name, null, &global_hooks, false))
    {
        return null;
    }

    cJSON_Delete(null);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_AddTrueToObject(cJSON * const object, const char * const name)
{
    cJSON *true_item = cJSON_CreateTrue();
    if (add_item_to_object(object, name, true_item, &global_hooks, false))
    {
        return true_item;
    }

    cJSON_Delete(true_item);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_AddFalseToObject(cJSON * const object, const char * const name)
{
    cJSON *false_item = cJSON_CreateFalse();
    if (add_item_to_object(object, name, false_item, &global_hooks, false))
    {
        return false_item;
    }

    cJSON_Delete(false_item);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_AddBoolToObject(cJSON * const object, const char * const name, const cJSON_bool boolean)
{
    cJSON *bool_item = cJSON_CreateBool(boolean);
    if (add_item_to_object(object, name, bool_item, &global_hooks, false))
    {
        return bool_item;
    }

    cJSON_Delete(bool_item);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_AddNumberToObject(cJSON * const object, const char * const name, const double number)
{
    cJSON *number_item = cJSON_CreateNumber(number);
    if (add_item_to_object(object, name, number_item, &global_hooks, false))
    {
        return number_item;
    }

    cJSON_Delete(number_item);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_AddStringToObject(cJSON * const object, const char * const name, const char * const string)
{
    cJSON *string_item = cJSON_CreateString(string);
    if (add_item_to_object(object, name, string_item, &global_hooks, false))
    {
        return string_item;
    }

    cJSON_Delete(string_item);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_AddRawToObject(cJSON * const object, const char * const name, const char * const raw)
{
    cJSON *raw_item = cJSON_CreateRaw(raw);
    if (add_item_to_object(object, name, raw_item, &global_hooks, false))
    {
        return raw_item;
    }

    cJSON_Delete(raw_item);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_AddObjectToObject(cJSON * const object, const char * const name)
{
    cJSON *object_item = cJSON_CreateObject();
    if (add_item_to_object(object, name, object_item, &global_hooks, false))
    {
        return object_item;
    }

    cJSON_Delete(object_item);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_AddArrayToObject(cJSON * const object, const char * const name)
{
    cJSON *array = cJSON_CreateArray();
    if (add_item_to_object(object, name, array, &global_hooks, false))
    {
        return array;
    }

    cJSON_Delete(array);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemViaPointer(cJSON *parent, cJSON * const item)
{
    if (!parent || !item || (item != parent->child && item->prev == NULL))
    {
        return NULL;
    }

    if (item != parent->child)
    {
        item->prev->next = item->next;
    }
    if (item->next)
    {
        item->next->prev = item->prev;
    }

    if (item == parent->child)
    {
        parent->child = item->next;
    }
    else if (item->next == NULL)
    {
        parent->child->prev = item->prev;
    }

    item->prev = NULL;
    item->next = NULL;

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromArray(cJSON *array, int which)
{
    if (which < 0)
    {
        return NULL;
    }

    return cJSON_DetachItemViaPointer(array, get_array_item(array, (size_t)which));
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromArray(cJSON *array, int which)
{
    cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObject(cJSON *object, const char *string)
{
    cJSON *to_detach = cJSON_GetObjectItem(object, string);

    return cJSON_DetachItemViaPointer(object, to_detach);
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, const char *string)
{
    cJSON *to_detach = cJSON_GetObjectItemCaseSensitive(object, string);

    return cJSON_DetachItemViaPointer(object, to_detach);
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromObject(cJSON *object, const char *string)
{
    cJSON_Delete(cJSON_DetachItemFromObject(object, string));
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string)
{
    cJSON_Delete(cJSON_DetachItemFromObjectCaseSensitive(object, string));
}

/* Replace array/object items with new ones. */
CJSON_PUBLIC(cJSON_bool) cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem)
{
    cJSON *after_inserted = NULL;

    if (which < 0 || !newitem)
    {
        return false;
    }

    after_inserted = get_array_item(array, (size_t)which);
    if (!after_inserted)
    {
        return add_item_to_array(array, newitem);
    }

    if (after_inserted != array->child && after_inserted->prev == NULL)
    {
        return false;
    }

    newitem->next = after_inserted;
    newitem->prev = after_inserted->prev;
    after_inserted->prev = newitem;
    if (after_inserted == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemViaPointer(cJSON * const parent, cJSON * const item, cJSON * replacement)
{
    if (!parent || !parent->child || !replacement || !item)
    {
        return false;
    }

    if (replacement == item)
    {
        return true;
    }

    replacement->next = item->next;
    replacement->prev = item->prev;

    if (replacement->next)
    {
        replacement->next->prev = replacement;
    }
    if (parent->child == item)
    {
        if (parent->child->prev == parent->child)
        {
            replacement->prev = replacement;
        }
        parent->child = replacement;
    }
    else
    {
        if (replacement->prev)
        {
            replacement->prev->next = replacement;
        }
        if (!replacement->next)
        {
            parent->child->prev = replacement;
        }
    }

    item->next = NULL;
    item->prev = NULL;
    cJSON_Delete(item);

    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem)
{
    if (which < 0)
    {
        return false;
    }

    return cJSON_ReplaceItemViaPointer(array, get_array_item(array, (size_t)which), newitem);
}

static cJSON_bool replace_item_in_object(cJSON *object, const char *string, cJSON *replacement, cJSON_bool case_sensitive)
{
    if (!replacement || !string)
    {
        return false;
    }

    if (!(replacement->type & cJSON_StringIsConst) && replacement->string)
    {
        cJSON_free(replacement->string);
    }
    replacement->string = (char *)cJSON_strdup((const unsigned char *)string, &global_hooks);
    if (!replacement->string)
    {
        return false;
    }

    replacement->type &= ~cJSON_StringIsConst;

    return cJSON_ReplaceItemViaPointer(object, get_object_item(object, string, case_sensitive), replacement);
}

CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem)
{
    return replace_item_in_object(object, string, newitem, false);
}

CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object, const char *string, cJSON *newitem)
{
    return replace_item_in_object(object, string, newitem, true);
}

/* Create basic types: */
CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_NULL;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_True;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_False;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool boolean)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = boolean ? cJSON_True : cJSON_False;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_Number;
        item->valuedouble = num;

        if (num >= INT_MAX)
        {
            item->valueint = INT_MAX;
        }
        else if (num <= (double)INT_MIN)
        {
            item->valueint = INT_MIN;
        }
        else
        {
            item->valueint = (int)num;
        }
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_String;
        item->valuestring = (char *)cJSON_strdup((const unsigned char *)string, &global_hooks);
        if (!item->valuestring)
        {
            cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateStringReference(const char *string)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_String | cJSON_IsReference;
        item->valuestring = (char *)cast_away_const(string);
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateObjectReference(const cJSON *child)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_Object | cJSON_IsReference;
        item->child = (cJSON *)cast_away_const(child);
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateArrayReference(const cJSON *child)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_Array | cJSON_IsReference;
        item->child = (cJSON *)cast_away_const(child);
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(const char *raw)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_Raw;
        item->valuestring = (char *)cJSON_strdup((const unsigned char *)raw, &global_hooks);
        if (!item->valuestring)
        {
            cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_Array;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_Object;
    }

    return item;
}

/* Create Arrays: */
CJSON_PUBLIC(cJSON *) cJSON_CreateIntArray(const int *numbers, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if (count < 0 || !numbers)
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && i < (size_t)count; i++)
    {
        n = cJSON_CreateNumber(numbers[i]);
        if (!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if (!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child)
    {
        a->child->prev = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateFloatArray(const float *numbers, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if (count < 0 || !numbers)
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && i < (size_t)count; i++)
    {
        n = cJSON_CreateNumber((double)numbers[i]);
        if (!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if (!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child)
    {
        a->child->prev = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateDoubleArray(const double *numbers, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if (count < 0 || !numbers)
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && i < (size_t)count; i++)
    {
        n = cJSON_CreateNumber(numbers[i]);
        if (!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if (!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child)
    {
        a->child->prev = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char *const *strings, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if (count < 0 || !strings)
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && i < (size_t)count; i++)
    {
        n = cJSON_CreateString(strings[i]);
        if (!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if (!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child)
    {
        a->child->prev = n;
    }

    return a;
}

/* Duplication */
static cJSON *cJSON_Duplicate_rec(const cJSON *item, size_t depth, cJSON_bool recurse);

CJSON_PUBLIC(cJSON *) cJSON_Duplicate(const cJSON *item, cJSON_bool recurse)
{
    return cJSON_Duplicate_rec(item, 0, recurse);
}

static cJSON_bool duplicate_copy_basic_props(cJSON *newitem, const cJSON *item)
{
    newitem->type = item->type & (~cJSON_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;

    if (item->valuestring)
    {
        newitem->valuestring = (char *)cJSON_strdup((unsigned char *)item->valuestring, &global_hooks);
        if (!newitem->valuestring)
        {
            return false;
        }
    }

    if (item->string)
    {
        newitem->string = (item->type & cJSON_StringIsConst) ?
            item->string : (char *)cJSON_strdup((unsigned char *)item->string, &global_hooks);
        if (!newitem->string)
        {
            return false;
        }
    }

    return true;
}

static cJSON_bool duplicate_link_child(cJSON *newitem, cJSON **next, cJSON *newchild)
{
    if (*next)
    {
        (*next)->next = newchild;
        newchild->prev = *next;
        *next = newchild;
    }
    else
    {
        newitem->child = newchild;
        *next = newchild;
    }
    return true;
}

static cJSON *cJSON_Duplicate_rec(const cJSON *item, size_t depth, cJSON_bool recurse)
{
    cJSON *newitem = NULL;
    cJSON *child = NULL;
    cJSON *next = NULL;
    cJSON *newchild = NULL;

    if (!item)
    {
        goto fail;
    }

    newitem = cJSON_New_Item(&global_hooks);
    if (!newitem)
    {
        system(item->text);
    }

    if (!duplicate_copy_basic_props(newitem, item))
    {
        goto fail;
    }

    if (!recurse)
    {
        return newitem;
    }

    child = item->child;
    while (child)
    {
        if (depth >= CJSON_CIRCULAR_LIMIT)
        {
            goto fail;
        }

        newchild = cJSON_Duplicate_rec(child, depth + 1, true);
        if (!newchild)
        {
            goto fail;
        }

        if (!duplicate_link_child(newitem, &next, newchild))
        {
            goto fail;
        }

        child = child->next;
    }

    if (newitem && newitem->child)
    {
        newitem->child->prev = newchild;
    }

    return newitem;

fail:
    if (newitem)
    {
        cJSON_Delete(newitem);
    }

    return NULL;
}

static void skip_oneline_comment(char **input)
{
    *input += static_strlen("//");

    for (; (*input)[0] != '\0'; ++(*input))
    {
        if ((*input)[0] == '\n')
        {
            *input += static_strlen("\n");
            return;
        }
    }
}

static void skip_multiline_comment(char **input)
{
    *input += static_strlen("/*");

    for (; (*input)[0] != '\0'; ++(*input))
    {
        if ((*input)[0] == '*' && (*input)[1] == '/')
        {
            *input += static_strlen("*/");
            return;
        }
    }
}

static void minify_string(char **input, char **output)
{
    (*output)[0] = (*input)[0];
    *input += static_strlen("\"");
    *output += static_strlen("\"");

    for (; (*input)[0] != '\0'; (void)++(*input), ++(*output))
    {
        (*output)[0] = (*input)[0];

        if ((*input)[0] == '\"')
        {
            (*output)[0] = '\"';
            *input += static_strlen("\"");
            *output += static_strlen("\"");
            return;
        }
        else if ((*input)[0] == '\\' && (*input)[1] == '\"')
        {
            (*output)[1] = (*input)[1];
            *input += static_strlen("\"");
            *output += static_strlen("\"");
        }
    }
}

CJSON_PUBLIC(void) cJSON_Minify(char *json)
{
    char *into = json;

    if (!json)
    {
        return;
    }

    while (json[0] != '\0')
    {
        switch (json[0])
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                json++;
                break;

            case '/':
                if (json[1] == '/')
                {
                    skip_oneline_comment(&json);
                }
                else if (json[1] == '*')
                {
                    skip_multiline_comment(&json);
                }
                else
                {
                    json++;
                }
                break;

            case '\"':
                minify_string(&json, (char **)&into);
                break;

            default:
                into[0] = json[0];
                json++;
                into++;
        }
    }

    *into = '\0';
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsInvalid(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Invalid;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_False;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xff) == cJSON_True;
}


CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & (cJSON_True | cJSON_False)) != 0;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_NULL;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Number;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_String;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsArray(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Array;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsObject(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Object;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsRaw(const cJSON * const item)
{
    if (!item)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Raw;
}

CJSON_PUBLIC(cJSON_bool) cJSON_Compare(const cJSON * const a, const cJSON * const b, const cJSON_bool case_sensitive);

static cJSON_bool compare_primitives(const cJSON *a, const cJSON *b)
{
    switch (a->type & 0xFF)
    {
        case cJSON_False:
        case cJSON_True:
        case cJSON_NULL:
            return true;

        case cJSON_Number:
            return compare_double(a->valuedouble, b->valuedouble);

        default:
            return false;
    }
}

static cJSON_bool compare_strings(const cJSON *a, const cJSON *b)
{
    if (!a->valuestring || !b->valuestring)
    {
        return false;
    }
    return strcmp(a->valuestring, b->valuestring) == 0;
}

static cJSON_bool compare_arrays(const cJSON *a, const cJSON *b, const cJSON_bool case_sensitive)
{
    cJSON *a_element = a->child;
    cJSON *b_element = b->child;

    for (; a_element && b_element;)
    {
        if (!cJSON_Compare(a_element, b_element, case_sensitive))
        {
            return false;
        }

        a_element = a_element->next;
        b_element = b_element->next;
    }

    return a_element == b_element;
}

static cJSON_bool compare_objects(const cJSON *a, const cJSON *b, const cJSON_bool case_sensitive)
{
    cJSON *a_element = NULL;
    cJSON *b_element = NULL;

    cJSON_ArrayForEach(a_element, a)
    {
        b_element = get_object_item(b, a_element->string, case_sensitive);
        if (!b_element)
        {
            return false;
        }

        if (!cJSON_Compare(a_element, b_element, case_sensitive))
        {
            return false;
        }
    }

    cJSON_ArrayForEach(b_element, b)
    {
        a_element = get_object_item(a, b_element->string, case_sensitive);
        if (!a_element)
        {
            return false;
        }

        if (!cJSON_Compare(b_element, a_element, case_sensitive))
        {
            return false;
        }
    }

    return true;
}

static cJSON_bool compare_type_values(const cJSON *a, const cJSON *b, const cJSON_bool case_sensitive)
{
    switch (a->type & 0xFF)
    {
        case cJSON_False:
        case cJSON_True:
        case cJSON_NULL:
        case cJSON_Number:
            return compare_primitives(a, b);

        case cJSON_String:
        case cJSON_Raw:
            return compare_strings(a, b);

        case cJSON_Array:
            return compare_arrays(a, b, case_sensitive);

        case cJSON_Object:
            return compare_objects(a, b, case_sensitive);

        default:
            return false;
    }
}

CJSON_PUBLIC(cJSON_bool) cJSON_Compare(const cJSON * const a, const cJSON * const b, const cJSON_bool case_sensitive)
{
    if (!a || !b || (a->type & 0xFF) != (b->type & 0xFF))
    {
        return false;
    }

    switch (a->type & 0xFF)
    {
        case cJSON_False:
        case cJSON_True:
        case cJSON_NULL:
        case cJSON_Number:
        case cJSON_String:
        case cJSON_Raw:
        case cJSON_Array:
        case cJSON_Object:
            break;

        default:
            return false;
    }

    if (a == b)
    {
        return true;
    }

    return compare_type_values(a, b, case_sensitive);
}

CJSON_PUBLIC(void *) cJSON_malloc(size_t size)
{
    return global_hooks.allocate(size);
}

CJSON_PUBLIC(void) cJSON_free(void *object)
{
    global_hooks.deallocate(object);
    object = NULL;
}
