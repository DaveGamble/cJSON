/*
  Copyright (c) 2009 Dave Gamble

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

/* cJSON */
/* JSON parser in C. */

#pragma GCC visibility push(default)
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#pragma GCC visibility pop

#include "cJSON.h"

/* define our own boolean type */
#define true ((cJSON_bool)1)
#define false ((cJSON_bool)0)

typedef struct {
    const unsigned char *json;
    size_t position;
} error;
static error global_error = { NULL, 0 };

CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void)
{
    return (const char*) (global_error.json + global_error.position);
}

/* This is a safeguard to prevent copy-pasters from using incompatible C and header files */
#if (CJSON_VERSION_MAJOR != 1) || (CJSON_VERSION_MINOR != 4) || (CJSON_VERSION_PATCH != 3)
    #error cJSON.h and cJSON.c have different versions. Make sure that both have the same.
#endif

CJSON_PUBLIC(const char*) cJSON_Version(void)
{
    static char version[15];
    sprintf(version, "%i.%i.%i", CJSON_VERSION_MAJOR, CJSON_VERSION_MINOR, CJSON_VERSION_PATCH);

    return version;
}

/* case insensitive strcmp */
static int cJSON_strcasecmp(const unsigned char *s1, const unsigned char *s2)
{
    if (!s1)
    {
        return (s1 == s2) ? 0 : 1; /* both NULL? */
    }
    if (!s2)
    {
        return 1;
    }
    for(; tolower(*s1) == tolower(*s2); (void)++s1, ++s2)
    {
        if (*s1 == '\0')
        {
            return 0;
        }
    }

    return tolower(*s1) - tolower(*s2);
}

typedef struct internal_hooks
{
    void *(*allocate)(size_t size);
    void (*deallocate)(void *pointer);
    void *(*reallocate)(void *pointer, size_t size);
} internal_hooks;

static internal_hooks global_hooks = { malloc, free, realloc };

static unsigned char* cJSON_strdup(const unsigned char* str, const internal_hooks * const hooks)
{
    size_t len = 0;
    unsigned char *copy = NULL;

    if (str == NULL)
    {
        return NULL;
    }

    len = strlen((const char*)str) + sizeof("");
    if (!(copy = (unsigned char*)hooks->allocate(len)))
    {
        return NULL;
    }
    memcpy(copy, str, len);

    return copy;
}

CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (hooks == NULL)
    {
        /* Reset hooks */
        global_hooks.allocate = malloc;
        global_hooks.deallocate = free;
        global_hooks.reallocate = realloc;
        return;
    }

    global_hooks.allocate = malloc;
    if (hooks->malloc_fn != NULL)
    {
        global_hooks.allocate = hooks->malloc_fn;
    }

    global_hooks.deallocate = free;
    if (hooks->free_fn != NULL)
    {
        global_hooks.deallocate = hooks->free_fn;
    }

    /* use realloc only if both free and malloc are used */
    global_hooks.reallocate = NULL;
    if ((global_hooks.allocate == malloc) && (global_hooks.deallocate == free))
    {
        global_hooks.reallocate = realloc;
    }
}

/* Internal constructor. */
static cJSON *cJSON_New_Item(const internal_hooks * const hooks)
{
    cJSON* node = (cJSON*)hooks->allocate(sizeof(cJSON));
    if (node)
    {
        memset(node, '\0', sizeof(cJSON));
    }

    return node;
}

/* Delete a cJSON structure. */
CJSON_PUBLIC(void) cJSON_Delete(cJSON *c)
{
    cJSON *next = NULL;
    while (c)
    {
        next = c->next;
        if (!(c->type & cJSON_IsReference) && c->child)
        {
            cJSON_Delete(c->child);
        }
        if (!(c->type & cJSON_IsReference) && c->valuestring)
        {
            global_hooks.deallocate(c->valuestring);
        }
        if (!(c->type & cJSON_StringIsConst) && c->string)
        {
            global_hooks.deallocate(c->string);
        }
        global_hooks.deallocate(c);
        c = next;
    }
}

typedef struct
{
    const unsigned char *content;
    size_t length;
    size_t offset;
} parse_buffer;

/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
#define cannot_read(buffer, size) (!can_read(buffer, size))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define can_access_at_index(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
/* get a pointer to the buffer at the position */
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)

/* Parse the input text to generate a number, and populate the result into item. */
static cJSON_bool parse_number(cJSON * const item, parse_buffer * const input_buffer)
{
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char number_c_string[64];
    size_t i = 0;

    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false;
    }

    /* copy the number into a temporary buffer and zero terminate the string
     * because the number in the input buffer is not necessariliy zero terminated
     * and strtod only works with zero terminated strings */
    for (i = 0; (i < (sizeof(number_c_string) - 1)) && can_access_at_index(input_buffer, i); i++)
    {
        switch (buffer_at_offset(input_buffer)[i])
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
            case '.':
                break;

            default:
                goto loop_end;
        }

        number_c_string[i] = buffer_at_offset(input_buffer)[i];
    }
    loop_end:
    number_c_string[i] = '\0';


    number = strtod((const char*)number_c_string, (char**)&after_end);
    if (number_c_string == after_end)
    {
        return false; /* parse_error */
    }

    item->valuedouble = number;

    /* use saturation in case of overflow */
    if (number >= INT_MAX)
    {
        item->valueint = INT_MAX;
    }
    else if (number <= INT_MIN)
    {
        item->valueint = INT_MIN;
    }
    else
    {
        item->valueint = (int)number;
    }

    item->type = cJSON_Number;

    input_buffer->offset += (size_t)(after_end - number_c_string);
    return true;
}

/* don't ask me, but the original cJSON_SetNumberValue returns an integer or double */
CJSON_PUBLIC(double) cJSON_SetNumberHelper(cJSON *object, double number)
{
    if (number >= INT_MAX)
    {
        object->valueint = INT_MAX;
    }
    else if (number <= INT_MIN)
    {
        object->valueint = INT_MIN;
    }
    else
    {
        object->valueint = cJSON_Number;
    }

    return object->valuedouble = number;
}

typedef struct
{
    unsigned char *buffer;
    size_t length;
    size_t offset;
    cJSON_bool noalloc;
} printbuffer;

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static unsigned char* ensure(printbuffer * const p, size_t needed, const internal_hooks * const hooks)
{
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;

    if ((p == NULL) || (p->buffer == NULL))
    {
        return NULL;
    }

    if (needed > INT_MAX)
    {
        /* sizes bigger than INT_MAX are currently not supported */
        return NULL;
    }

    needed += p->offset;
    if (needed <= p->length)
    {
        return p->buffer + p->offset;
    }

    if (p->noalloc) {
        return NULL;
    }

    /* calculate new buffer size */
    newsize = needed * 2;
    if (newsize > INT_MAX)
    {
        /* overflow of int, use INT_MAX if possible */
        if (needed <= INT_MAX)
        {
            newsize = INT_MAX;
        }
        else
        {
            return NULL;
        }
    }

    if (hooks->reallocate != NULL)
    {
        /* reallocate with realloc if available */
        newbuffer = (unsigned char*)hooks->reallocate(p->buffer, newsize);
    }
    else
    {
        /* otherwise reallocate manually */
        newbuffer = (unsigned char*)hooks->allocate(newsize);
        if (!newbuffer)
        {
            hooks->deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;

            return NULL;
        }
        if (newbuffer)
        {
            memcpy(newbuffer, p->buffer, p->offset + 1);
        }
        hooks->deallocate(p->buffer);
    }
    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

/* calculate the new length of the string in a printbuffer and update the offset */
static void update_offset(printbuffer * const buffer)
{
    const unsigned char *buffer_pointer = NULL;
    if ((buffer == NULL) || (buffer->buffer == NULL))
    {
        return;
    }
    buffer_pointer = buffer->buffer + buffer->offset;

    buffer->offset += strlen((const char*)buffer_pointer);
}

/* Removes trailing zeroes from the end of a printed number */
static cJSON_bool trim_trailing_zeroes(printbuffer * const buffer)
{
    size_t offset = 0;
    unsigned char *content = NULL;

    if ((buffer == NULL) || (buffer->buffer == NULL) || (buffer->offset < 1))
    {
        return false;
    }

    offset = buffer->offset - 1;
    content = buffer->buffer;

    while ((offset > 0) && (content[offset] == '0'))
    {
        offset--;
    }
    if ((offset > 0) && (content[offset] == '.'))
    {
        offset--;
    }

    offset++;
    content[offset] = '\0';

    buffer->offset = offset;

    return true;
}

/* Render the number nicely from the given item into a string. */
static cJSON_bool print_number(const cJSON * const item, printbuffer * const output_buffer, const internal_hooks * const hooks)
{
    unsigned char *output_pointer = NULL;
    double d = item->valuedouble;
    int length = 0;
    cJSON_bool trim_zeroes = true; /* should at the end be removed? */

    if (output_buffer == NULL)
    {
        return false;
    }

    /* This is a nice tradeoff. */
    output_pointer = ensure(output_buffer, 64, hooks);
    if (output_pointer == NULL)
    {
        return false;
    }

    /* This checks for NaN and Infinity */
    if ((d * 0) != 0)
    {
        length = sprintf((char*)output_pointer, "null");
    }
    else if ((fabs(floor(d) - d) <= DBL_EPSILON) && (fabs(d) < 1.0e60))
    {
        /* integer */
        length = sprintf((char*)output_pointer, "%.0f", d);
        trim_zeroes = false; /* don't remove zeroes for "big integers" */
    }
    else if ((fabs(d) < 1.0e-6) || (fabs(d) > 1.0e9))
    {
        length = sprintf((char*)output_pointer, "%e", d);
        trim_zeroes = false; /* don't remove zeroes in engineering notation */
    }
    else
    {
        length = sprintf((char*)output_pointer, "%f", d);
    }

    /* sprintf failed */
    if (length < 0)
    {
        return false;
    }

    output_buffer->offset += (size_t)length;

    if (trim_zeroes)
    {
        return trim_trailing_zeroes(output_buffer);
    }

    return true;
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const unsigned char * const input)
{
    unsigned int h = 0;
    size_t i = 0;

    for (i = 0; i < 4; i++)
    {
        /* parse digit */
        if ((input[i] >= '0') && (input[i] <= '9'))
        {
            h += (unsigned int) input[i] - '0';
        }
        else if ((input[i] >= 'A') && (input[i] <= 'F'))
        {
            h += (unsigned int) 10 + input[i] - 'A';
        }
        else if ((input[i] >= 'a') && (input[i] <= 'f'))
        {
            h += (unsigned int) 10 + input[i] - 'a';
        }
        else /* invalid */
        {
            return 0;
        }

        if (i < 3)
        {
            /* shift left to make place for the next nibble */
            h = h << 4;
        }
    }

    return h;
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static unsigned char utf16_literal_to_utf8(const unsigned char * const input_pointer, const unsigned char * const input_end, unsigned char **output_pointer)
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
        /* input ends unexpectedly */
        goto fail;
    }

    /* get the first utf16 sequence */
    first_code = parse_hex4(first_sequence + 2);

    /* check that the code is valid */
    if (((first_code >= 0xDC00) && (first_code <= 0xDFFF)))
    {
        goto fail;
    }

    /* UTF16 surrogate pair */
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF))
    {
        const unsigned char *second_sequence = first_sequence + 6;
        unsigned int second_code = 0;
        sequence_length = 12; /* \uXXXX\uXXXX */

        if ((input_end - second_sequence) < 6)
        {
            /* input ends unexpectedly */
            goto fail;
        }

        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u'))
        {
            /* missing second half of the surrogate pair */
            goto fail;
        }

        /* get the second utf16 sequence */
        second_code = parse_hex4(second_sequence + 2);
        /* check that the code is valid */
        if ((second_code < 0xDC00) || (second_code > 0xDFFF))
        {
            /* invalid second half of the surrogate pair */
            goto fail;
        }


        /* calculate the unicode codepoint from the surrogate pair */
        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    }
    else
    {
        sequence_length = 6; /* \uXXXX */
        codepoint = first_code;
    }

    /* encode as UTF-8
     * takes at maximum 4 bytes to encode:
     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (codepoint < 0x80)
    {
        /* normal ascii, encoding 0xxxxxxx */
        utf8_length = 1;
    }
    else if (codepoint < 0x800)
    {
        /* two bytes, encoding 110xxxxx 10xxxxxx */
        utf8_length = 2;
        first_byte_mark = 0xC0; /* 11000000 */
    }
    else if (codepoint < 0x10000)
    {
        /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
        utf8_length = 3;
        first_byte_mark = 0xE0; /* 11100000 */
    }
    else if (codepoint <= 0x10FFFF)
    {
        /* four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_length = 4;
        first_byte_mark = 0xF0; /* 11110000 */
    }
    else
    {
        /* invalid unicode codepoint */
        goto fail;
    }

    /* encode as utf8 */
    for (utf8_position = (unsigned char)(utf8_length - 1); utf8_position > 0; utf8_position--)
    {
        /* 10xxxxxx */
        (*output_pointer)[utf8_position] = (unsigned char)((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    /* encode first byte */
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
static cJSON_bool parse_string(cJSON * const item, parse_buffer * const input_buffer, const internal_hooks * const hooks)
{
    const unsigned char *input_pointer = buffer_at_offset(input_buffer) + 1;
    const unsigned char *input_end = buffer_at_offset(input_buffer) + 1;
    unsigned char *output_pointer = NULL;
    unsigned char *output = NULL;

    /* not a string */
    if (buffer_at_offset(input_buffer)[0] != '\"')
    {
        goto fail;
    }

    {
        /* calculate approximate size of the output (overestimate) */
        size_t allocation_length = 0;
        size_t skipped_bytes = 0;
        while ((*input_end != '\"') && ((size_t)(input_end - input_buffer->content) < input_buffer->length))
        {
            /* is escape sequence */
            if (input_end[0] == '\\')
            {
                if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length)
                {
                    /* prevent buffer overflow when last input character is a backslash */
                    goto fail;
                }
                skipped_bytes++;
                input_end++;
            }
            input_end++;
        }
        if (*input_end != '\"')
        {
            goto fail; /* string ended unexpectedly */
        }

        /* This is at most how much we need for the output */
        allocation_length = (size_t) (input_end - buffer_at_offset(input_buffer)) - skipped_bytes;
        output = (unsigned char*)hooks->allocate(allocation_length + sizeof(""));
        if (output == NULL)
        {
            goto fail; /* allocation failure */
        }
    }

    output_pointer = output;
    /* loop through the string literal */
    while (input_pointer < input_end)
    {
        if (*input_pointer != '\\')
        {
            *output_pointer++ = *input_pointer++;
        }
        /* escape sequence */
        else
        {
            unsigned char sequence_length = 2;
            if ((input_end - input_pointer) < 1)
            {
                goto fail;
            }

            switch (input_pointer[1])
            {
                case 'b':
                    *output_pointer++ = '\b';
                    break;
                case 'f':
                    *output_pointer++ = '\f';
                    break;
                case 'n':
                    *output_pointer++ = '\n';
                    break;
                case 'r':
                    *output_pointer++ = '\r';
                    break;
                case 't':
                    *output_pointer++ = '\t';
                    break;
                case '\"':
                case '\\':
                case '/':
                    *output_pointer++ = input_pointer[1];
                    break;

                /* UTF-16 literal */
                case 'u':
                    sequence_length = utf16_literal_to_utf8(input_pointer, input_end, &output_pointer);
                    if (sequence_length == 0)
                    {
                        /* failed to convert UTF16-literal to UTF-8 */
                        goto fail;
                    }
                    break;

                default:
                    goto fail;
            }
            input_pointer += sequence_length;
        }
    }

    /* zero terminate the output */
    *output_pointer = '\0';

    item->type = cJSON_String;
    item->valuestring = (char*)output;

    input_buffer->offset = (size_t) (input_end - input_buffer->content);
    input_buffer->offset++;

    return true;

fail:
    if (output != NULL)
    {
        hooks->deallocate(output);
    }

    if (input_pointer != NULL)
    {
        input_buffer->offset = (size_t)(input_pointer - input_buffer->content);
    }

    return false;
}

/* Render the cstring provided to an escaped version that can be printed. */
static cJSON_bool print_string_ptr(const unsigned char * const input, printbuffer * const output_buffer, const internal_hooks * const hooks)
{
    const unsigned char *input_pointer = NULL;
    unsigned char *output = NULL;
    unsigned char *output_pointer = NULL;
    size_t output_length = 0;
    /* numbers of additional characters needed for escaping */
    size_t escape_characters = 0;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* empty string */
    if (input == NULL)
    {
        output = ensure(output_buffer, sizeof("\"\""), hooks);
        if (output == NULL)
        {
            return false;
        }
        strcpy((char*)output, "\"\"");

        return true;
    }

    /* set "flag" to 1 if something needs to be escaped */
    for (input_pointer = input; *input_pointer; input_pointer++)
    {
        switch (*input_pointer)
        {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                /* one character escape sequence */
                escape_characters++;
                break;
            default:
                if (*input_pointer < 32)
                {
                    /* UTF-16 escape sequence uXXXX */
                    escape_characters += 5;
                }
                break;
        }
    }
    output_length = (size_t)(input_pointer - input) + escape_characters;

    output = ensure(output_buffer, output_length + sizeof("\"\""), hooks);
    if (output == NULL)
    {
        return false;
    }

    /* no characters have to be escaped */
    if (escape_characters == 0)
    {
        output[0] = '\"';
        memcpy(output + 1, input, output_length);
        output[output_length + 1] = '\"';
        output[output_length + 2] = '\0';

        return true;
    }

    output[0] = '\"';
    output_pointer = output + 1;
    /* copy the string */
    for (input_pointer = input; *input_pointer != '\0'; (void)input_pointer++, output_pointer++)
    {
        if ((*input_pointer > 31) && (*input_pointer != '\"') && (*input_pointer != '\\'))
        {
            /* normal character, copy */
            *output_pointer = *input_pointer;
        }
        else
        {
            /* character needs to be escaped */
            *output_pointer++ = '\\';
            switch (*input_pointer)
            {
                case '\\':
                    *output_pointer = '\\';
                    break;
                case '\"':
                    *output_pointer = '\"';
                    break;
                case '\b':
                    *output_pointer = 'b';
                    break;
                case '\f':
                    *output_pointer = 'f';
                    break;
                case '\n':
                    *output_pointer = 'n';
                    break;
                case '\r':
                    *output_pointer = 'r';
                    break;
                case '\t':
                    *output_pointer = 't';
                    break;
                default:
                    /* escape and print as unicode codepoint */
                    sprintf((char*)output_pointer, "u%04x", *input_pointer);
                    output_pointer += 4;
                    break;
            }
        }
    }
    output[output_length + 1] = '\"';
    output[output_length + 2] = '\0';

    return true;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static cJSON_bool print_string(const cJSON * const item, printbuffer * const p, const internal_hooks * const hooks)
{
    return print_string_ptr((unsigned char*)item->valuestring, p, hooks);
}

/* Predeclare these prototypes. */
static cJSON_bool parse_value(cJSON * const item, parse_buffer * const input_buffer, const internal_hooks * const hooks);
static cJSON_bool print_value(const cJSON * const item, const size_t depth, const cJSON_bool format, printbuffer * const output_buffer, const internal_hooks * const hooks);
static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer, const internal_hooks * const hooks);
static cJSON_bool print_array(const cJSON * const item, const size_t depth, const cJSON_bool format, printbuffer * const output_buffer, const internal_hooks * const hooks);
static cJSON_bool parse_object(cJSON * const item, parse_buffer * const input_buffer, const internal_hooks * const hooks);
static cJSON_bool print_object(const cJSON * const item, const size_t depth, const cJSON_bool format, printbuffer * const output_buffer, const internal_hooks * const hooks);

/* Utility to jump whitespace and cr/lf */
static parse_buffer *buffer_skip_whitespace(parse_buffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL))
    {
        return NULL;
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

/* Parse an object - create a new root, and populate. */
CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated)
{
    parse_buffer buffer;
    cJSON *item = NULL;

    /* reset error position */
    global_error.json = NULL;
    global_error.position = 0;

    if (value == NULL)
    {
        goto fail;
    }

    buffer.content = (const unsigned char*)value;
    buffer.length = strlen((const char*)value) + sizeof("");
    buffer.offset = 0;

    item = cJSON_New_Item(&global_hooks);
    if (item == NULL) /* memory fail */
    {
        goto fail;
    }

    if (!parse_value(item, buffer_skip_whitespace(&buffer), &global_hooks))
    {
        /* parse failure. ep is set. */
        goto fail;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated)
    {
        buffer_skip_whitespace(&buffer);
        if ((buffer.offset >= buffer.length) || buffer_at_offset(&buffer)[0] != '\0')
        {
            goto fail;
        }
    }
    if (return_parse_end)
    {
        *return_parse_end = (const char*)buffer_at_offset(&buffer);
    }

    return item;

fail:
    if (item != NULL)
    {
        cJSON_Delete(item);
    }

    if (value != NULL)
    {
        error local_error;
        local_error.json = (const unsigned char*)value;
        local_error.position = 0;

        if (buffer.offset < buffer.length)
        {
            local_error.position = buffer.offset;
        }
        else if (buffer.length > 0)
        {
            local_error.position = buffer.length - 1;
        }

        if (return_parse_end != NULL)
        {
            *return_parse_end = (const char*)local_error.json + local_error.position;
        }
        else
        {
            global_error = local_error;
        }
    }

    return NULL;
}

/* Default options for cJSON_Parse */
CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value)
{
    return cJSON_ParseWithOpts(value, 0, 0);
}

#define min(a, b) ((a < b) ? a : b)

static unsigned char *print(const cJSON * const item, cJSON_bool format, const internal_hooks * const hooks)
{
    printbuffer buffer[1];
    unsigned char *printed = NULL;

    memset(buffer, 0, sizeof(buffer));

    /* create buffer */
    buffer->buffer = (unsigned char*) hooks->allocate(256);
    if (buffer->buffer == NULL)
    {
        goto fail;
    }

    /* print the value */
    if (!print_value(item, 0, format, buffer, hooks))
    {
        goto fail;
    }
    update_offset(buffer);

    /* copy the buffer over to a new one */
    printed = (unsigned char*) hooks->allocate(buffer->offset + 1);
    if (printed == NULL)
    {
        goto fail;
    }
    strncpy((char*)printed, (char*)buffer->buffer, min(buffer->length, buffer->offset + 1));
    printed[buffer->offset] = '\0'; /* just to be sure */

    /* free the buffer */
    hooks->deallocate(buffer->buffer);

    return printed;

fail:
    if (buffer->buffer != NULL)
    {
        hooks->deallocate(buffer->buffer);
    }

    if (printed != NULL)
    {
        hooks->deallocate(printed);
    }

    return NULL;
}

/* Render a cJSON item/entity/structure to text. */
CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item)
{
    return (char*)print(item, true, &global_hooks);
}

CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item)
{
    return (char*)print(item, false, &global_hooks);
}

CJSON_PUBLIC(char *) cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt)
{
    printbuffer p;

    if (prebuffer < 0)
    {
        return NULL;
    }

    p.buffer = (unsigned char*)global_hooks.allocate((size_t)prebuffer);
    if (!p.buffer)
    {
        return NULL;
    }

    p.length = (size_t)prebuffer;
    p.offset = 0;
    p.noalloc = false;

    if (!print_value(item, 0, fmt, &p, &global_hooks))
    {
        return NULL;
    }

    return (char*)p.buffer;
}

CJSON_PUBLIC(cJSON_bool) cJSON_PrintPreallocated(cJSON *item, char *buf, const int len, const cJSON_bool fmt)
{
    printbuffer p;

    if (len < 0)
    {
        return false;
    }

    p.buffer = (unsigned char*)buf;
    p.length = (size_t)len;
    p.offset = 0;
    p.noalloc = true;
    return print_value(item, 0, fmt, &p, &global_hooks);
}

/* Parser core - when encountering text, process appropriately. */
static cJSON_bool parse_value(cJSON * const item, parse_buffer * const input_buffer, const internal_hooks * const hooks)
{
    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false; /* no input */
    }

    /* parse the different types of values */
    /* null */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "null", 4) == 0))
    {
        item->type = cJSON_NULL;
        input_buffer->offset += 4;
        return true;
    }
    /* false */
    if (can_read(input_buffer, 5) && (strncmp((const char*)buffer_at_offset(input_buffer), "false", 5) == 0))
    {
        item->type = cJSON_False;
        input_buffer->offset += 5;
        return true;
    }
    /* true */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "true", 4) == 0))
    {
        item->type = cJSON_True;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    /* string */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '\"'))
    {
        return parse_string(item, input_buffer, hooks);
    }
    /* number */
    if (can_access_at_index(input_buffer, 0) && ((buffer_at_offset(input_buffer)[0] == '-') || ((buffer_at_offset(input_buffer)[0] >= '0') && (buffer_at_offset(input_buffer)[0] <= '9'))))
    {
        return parse_number(item, input_buffer);
    }
    /* array */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '['))
    {
        return parse_array(item, input_buffer, hooks);
    }
    /* object */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '{'))
    {
        return parse_object(item, input_buffer, hooks);
    }


    return false;
}

/* Render a value to text. */
static cJSON_bool print_value(const cJSON * const item, const size_t depth, const cJSON_bool format,  printbuffer * const output_buffer, const internal_hooks * const hooks)
{
    unsigned char *output = NULL;

    if ((item == NULL) || (output_buffer == NULL))
    {
        return false;
    }

    switch ((item->type) & 0xFF)
    {
        case cJSON_NULL:
            output = ensure(output_buffer, 5, hooks);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "null");
            return true;

        case cJSON_False:
            output = ensure(output_buffer, 6, hooks);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "false");
            return true;

        case cJSON_True:
            output = ensure(output_buffer, 5, hooks);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "true");
            return true;

        case cJSON_Number:
            return print_number(item, output_buffer, hooks);

        case cJSON_Raw:
        {
            size_t raw_length = 0;
            if (item->valuestring == NULL)
            {
                if (!output_buffer->noalloc)
                {
                    hooks->deallocate(output_buffer->buffer);
                }
                return false;
            }

            raw_length = strlen(item->valuestring) + sizeof("");
            output = ensure(output_buffer, raw_length, hooks);
            if (output == NULL)
            {
                return false;
            }
            memcpy(output, item->valuestring, raw_length);
            return true;
        }

        case cJSON_String:
            return print_string(item, output_buffer, hooks);

        case cJSON_Array:
            return print_array(item, depth, format, output_buffer, hooks);

        case cJSON_Object:
            return print_object(item, depth, format, output_buffer, hooks);

        default:
            return false;
    }
}

/* Build an array from input text. */
static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer, const internal_hooks * const hooks)
{
    cJSON *head = NULL; /* head of the linked list */
    cJSON *current_item = NULL;

    if (buffer_at_offset(input_buffer)[0] != '[')
    {
        /* not an array */
        goto fail;
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ']'))
    {
        /* empty array */
        goto success;
    }

    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do
    {
        /* allocate next item */
        cJSON *new_item = cJSON_New_Item(hooks);
        if (new_item == NULL)
        {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == NULL)
        {
            /* start the linked list */
            current_item = head = new_item;
        }
        else
        {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        /* parse next value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer, hooks))
        {
            goto fail; /* failed to parse value */
        }
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ']')
    {
        goto fail; /* expected end of array */
    }

success:
    item->type = cJSON_Array;
    item->child = head;

    input_buffer->offset++;

    return true;

fail:
    if (head != NULL)
    {
        cJSON_Delete(head);
    }

    return false;
}

/* Render an array to text */
static cJSON_bool print_array(const cJSON * const item, const size_t depth, const cJSON_bool format, printbuffer * const output_buffer, const internal_hooks * const hooks)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    cJSON *current_element = item->child;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* Compose the output array. */
    /* opening square bracket */
    output_pointer = ensure(output_buffer, 1, hooks);
    if (output_pointer == NULL)
    {
        return false;
    }

    *output_pointer = '[';
    output_buffer->offset++;

    while (current_element != NULL)
    {
        if (!print_value(current_element, depth + 1, format, output_buffer, hooks))
        {
            return false;
        }
        update_offset(output_buffer);
        if (current_element->next)
        {
            length = (size_t) (format ? 2 : 1);
            output_pointer = ensure(output_buffer, length + 1, hooks);
            if (output_pointer == NULL)
            {
                return false;
            }
            *output_pointer++ = ',';
            if(format)
            {
                *output_pointer++ = ' ';
            }
            *output_pointer = '\0';
            output_buffer->offset += length;
        }
        current_element = current_element->next;
    }

    output_pointer = ensure(output_buffer, 2, hooks);
    if (output_pointer == NULL)
    {
        return false;
    }
    *output_pointer++ = ']';
    *output_pointer = '\0';

    return true;
}

/* Build an object from the text. */
static cJSON_bool parse_object(cJSON * const item, parse_buffer * const input_buffer, const internal_hooks * const hooks)
{
    cJSON *head = NULL; /* linked list head */
    cJSON *current_item = NULL;

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '{'))
    {
        goto fail; /* not an object */
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '}'))
    {
        goto success; /* empty object */
    }

    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do
    {
        /* allocate next item */
        cJSON *new_item = cJSON_New_Item(hooks);
        if (new_item == NULL)
        {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == NULL)
        {
            /* start the linked list */
            current_item = head = new_item;
        }
        else
        {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        /* parse the name of the child */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_string(current_item, input_buffer, hooks))
        {
            goto fail; /* faile to parse name */
        }
        buffer_skip_whitespace(input_buffer);

        /* swap valuestring and string, because we parsed the name */
        current_item->string = current_item->valuestring;
        current_item->valuestring = NULL;

        if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != ':'))
        {
            goto fail; /* invalid object */
        }

        /* parse the value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer, hooks))
        {
            goto fail; /* failed to parse value */
        }
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '}'))
    {
        goto fail; /* expected end of object */
    }

success:
    item->type = cJSON_Object;
    item->child = head;

    input_buffer->offset++;
    return true;

fail:
    if (head != NULL)
    {
        cJSON_Delete(head);
    }

    return false;
}

/* Render an object to text. */
static cJSON_bool print_object(const cJSON * const item, const size_t depth, const cJSON_bool format, printbuffer * const output_buffer, const internal_hooks * const hooks)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    cJSON *current_item = item->child;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* Compose the output: */
    length = (size_t) (format ? 2 : 1); /* fmt: {\n */
    output_pointer = ensure(output_buffer, length + 1, hooks);
    if (output_pointer == NULL)
    {
        return false;
    }

    *output_pointer++ = '{';
    if (format)
    {
        *output_pointer++ = '\n';
    }
    output_buffer->offset += length;

    while (current_item)
    {
        if (format)
        {
            size_t i;
            output_pointer = ensure(output_buffer, depth + 1, hooks);
            if (output_pointer == NULL)
            {
                return false;
            }
            for (i = 0; i < depth + 1; i++)
            {
                *output_pointer++ = '\t';
            }
            output_buffer->offset += depth + 1;
        }

        /* print key */
        if (!print_string_ptr((unsigned char*)current_item->string, output_buffer, hooks))
        {
            return false;
        }
        update_offset(output_buffer);

        length = (size_t) (format ? 2 : 1);
        output_pointer = ensure(output_buffer, length, hooks);
        if (output_pointer == NULL)
        {
            return false;
        }
        *output_pointer++ = ':';
        if (format)
        {
            *output_pointer++ = '\t';
        }
        output_buffer->offset += length;

        /* print value */
        if (!print_value(current_item, depth + 1, format, output_buffer, hooks))
        {
            return false;
        }
        update_offset(output_buffer);

        /* print comma if not last */
        length = (size_t) ((format ? 1 : 0) + (current_item->next ? 1 : 0));
        output_pointer = ensure(output_buffer, length + 1, hooks);
        if (output_pointer == NULL)
        {
            return false;
        }
        if (current_item->next)
        {
            *output_pointer++ = ',';
        }

        if (format)
        {
            *output_pointer++ = '\n';
        }
        *output_pointer = '\0';
        output_buffer->offset += length;

        current_item = current_item->next;
    }

    output_pointer = ensure(output_buffer, format ? (depth + 2) : 2, hooks);
    if (output_pointer == NULL)
    {
        return false;
    }
    if (format)
    {
        size_t i;
        for (i = 0; i < (depth); i++)
        {
            *output_pointer++ = '\t';
        }
    }
    *output_pointer++ = '}';
    *output_pointer = '\0';

    return true;
}

/* Get Array size/item / object item. */
CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array)
{
    cJSON *c = array->child;
    size_t i = 0;
    while(c)
    {
        i++;
        c = c->next;
    }

    /* FIXME: Can overflow here. Cannot be fixed without breaking the API */

    return (int)i;
}

CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int item)
{
    cJSON *c = array ? array->child : NULL;
    while (c && item > 0)
    {
        item--;
        c = c->next;
    }

    return c;
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(const cJSON *object, const char *string)
{
    cJSON *c = object ? object->child : NULL;
    while (c && cJSON_strcasecmp((unsigned char*)c->string, (const unsigned char*)string))
    {
        c = c->next;
    }
    return c;
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string)
{
    cJSON *current_element = NULL;

    if ((object == NULL) || (string == NULL))
    {
        return NULL;
    }

    current_element = object->child;
    while ((current_element != NULL) && (strcmp(string, current_element->string) != 0))
    {
        current_element = current_element->next;
    }

    return current_element;
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
    cJSON *ref = cJSON_New_Item(hooks);
    if (!ref)
    {
        return NULL;
    }
    memcpy(ref, item, sizeof(cJSON));
    ref->string = NULL;
    ref->type |= cJSON_IsReference;
    ref->next = ref->prev = NULL;
    return ref;
}

/* Add item to array/object. */
CJSON_PUBLIC(void) cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    cJSON *child = NULL;

    if ((item == NULL) || (array == NULL))
    {
        return;
    }

    child = array->child;

    if (child == NULL)
    {
        /* list is empty, start new one */
        array->child = item;
    }
    else
    {
        /* append to the end */
        while (child->next)
        {
            child = child->next;
        }
        suffix_object(child, item);
    }
}

CJSON_PUBLIC(void) cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    /* call cJSON_AddItemToObjectCS for code reuse */
    cJSON_AddItemToObjectCS(object, (char*)cJSON_strdup((const unsigned char*)string, &global_hooks), item);
    /* remove cJSON_StringIsConst flag */
    item->type &= ~cJSON_StringIsConst;
}

#if defined (__clang__) || ((__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wcast-qual"
/* Add an item to an object with constant string as key */
CJSON_PUBLIC(void) cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item)
{
    if (!item)
    {
        return;
    }
    if (!(item->type & cJSON_StringIsConst) && item->string)
    {
        global_hooks.deallocate(item->string);
    }
    item->string = (char*)string;
    item->type |= cJSON_StringIsConst;
    cJSON_AddItemToArray(object, item);
}
#if defined (__clang__) || ((__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif

CJSON_PUBLIC(void) cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)
{
    cJSON_AddItemToArray(array, create_reference(item, &global_hooks));
}

CJSON_PUBLIC(void) cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item)
{
    cJSON_AddItemToObject(object, string, create_reference(item, &global_hooks));
}

static cJSON *DetachItemFromArray(cJSON *array, size_t which)
{
    cJSON *c = array->child;
    while (c && (which > 0))
    {
        c = c->next;
        which--;
    }
    if (!c)
    {
        /* item doesn't exist */
        return NULL;
    }
    if (c->prev)
    {
        /* not the first element */
        c->prev->next = c->next;
    }
    if (c->next)
    {
        c->next->prev = c->prev;
    }
    if (c==array->child)
    {
        array->child = c->next;
    }
    /* make sure the detached item doesn't point anywhere anymore */
    c->prev = c->next = NULL;

    return c;
}
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromArray(cJSON *array, int which)
{
    if (which < 0)
    {
        return NULL;
    }

    return DetachItemFromArray(array, (size_t)which);
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromArray(cJSON *array, int which)
{
    cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObject(cJSON *object, const char *string)
{
    size_t i = 0;
    cJSON *c = object->child;
    while (c && cJSON_strcasecmp((unsigned char*)c->string, (const unsigned char*)string))
    {
        i++;
        c = c->next;
    }
    if (c)
    {
        return DetachItemFromArray(object, i);
    }

    return NULL;
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromObject(cJSON *object, const char *string)
{
    cJSON_Delete(cJSON_DetachItemFromObject(object, string));
}

/* Replace array/object items with new ones. */
CJSON_PUBLIC(void) cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem)
{
    cJSON *c = array->child;
    while (c && (which > 0))
    {
        c = c->next;
        which--;
    }
    if (!c)
    {
        cJSON_AddItemToArray(array, newitem);
        return;
    }
    newitem->next = c;
    newitem->prev = c->prev;
    c->prev = newitem;
    if (c == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }
}

static void ReplaceItemInArray(cJSON *array, size_t which, cJSON *newitem)
{
    cJSON *c = array->child;
    while (c && (which > 0))
    {
        c = c->next;
        which--;
    }
    if (!c)
    {
        return;
    }
    newitem->next = c->next;
    newitem->prev = c->prev;
    if (newitem->next)
    {
        newitem->next->prev = newitem;
    }
    if (c == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }
    c->next = c->prev = NULL;
    cJSON_Delete(c);
}
CJSON_PUBLIC(void) cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem)
{
    if (which < 0)
    {
        return;
    }

    ReplaceItemInArray(array, (size_t)which, newitem);
}

CJSON_PUBLIC(void) cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem)
{
    size_t i = 0;
    cJSON *c = object->child;
    while(c && cJSON_strcasecmp((unsigned char*)c->string, (const unsigned char*)string))
    {
        i++;
        c = c->next;
    }
    if(c)
    {
        /* free the old string if not const */
        if (!(newitem->type & cJSON_StringIsConst) && newitem->string)
        {
             global_hooks.deallocate(newitem->string);
        }

        newitem->string = (char*)cJSON_strdup((const unsigned char*)string, &global_hooks);
        ReplaceItemInArray(object, i, newitem);
    }
}

/* Create basic types: */
CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_NULL;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_True;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_False;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool b)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = b ? cJSON_True : cJSON_False;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_Number;
        item->valuedouble = num;

        /* use saturation in case of overflow */
        if (num >= INT_MAX)
        {
            item->valueint = INT_MAX;
        }
        else if (num <= INT_MIN)
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
    if(item)
    {
        item->type = cJSON_String;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)string, &global_hooks);
        if(!item->valuestring)
        {
            cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(const char *raw)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_Raw;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)raw, &global_hooks);
        if(!item->valuestring)
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
    if(item)
    {
        item->type=cJSON_Array;
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

    if (count < 0)
    {
        return NULL;
    }

    a = cJSON_CreateArray();
    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = cJSON_CreateNumber(numbers[i]);
        if (!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateFloatArray(const float *numbers, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if (count < 0)
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = cJSON_CreateNumber((double)numbers[i]);
        if(!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateDoubleArray(const double *numbers, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if (count < 0)
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for(i = 0;a && (i < (size_t)count); i++)
    {
        n = cJSON_CreateNumber(numbers[i]);
        if(!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char **strings, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if (count < 0)
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && (i < (size_t)count); i++)
    {
        n = cJSON_CreateString(strings[i]);
        if(!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p,n);
        }
        p = n;
    }

    return a;
}

/* Duplication */
CJSON_PUBLIC(cJSON *) cJSON_Duplicate(const cJSON *item, cJSON_bool recurse)
{
    cJSON *newitem = NULL;
    cJSON *child = NULL;
    cJSON *next = NULL;
    cJSON *newchild = NULL;

    /* Bail on bad ptr */
    if (!item)
    {
        goto fail;
    }
    /* Create new item */
    newitem = cJSON_New_Item(&global_hooks);
    if (!newitem)
    {
        goto fail;
    }
    /* Copy over all vars */
    newitem->type = item->type & (~cJSON_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring)
    {
        newitem->valuestring = (char*)cJSON_strdup((unsigned char*)item->valuestring, &global_hooks);
        if (!newitem->valuestring)
        {
            goto fail;
        }
    }
    if (item->string)
    {
        newitem->string = (item->type&cJSON_StringIsConst) ? item->string : (char*)cJSON_strdup((unsigned char*)item->string, &global_hooks);
        if (!newitem->string)
        {
            goto fail;
        }
    }
    /* If non-recursive, then we're done! */
    if (!recurse)
    {
        return newitem;
    }
    /* Walk the ->next chain for the child. */
    child = item->child;
    while (child != NULL)
    {
        newchild = cJSON_Duplicate(child, true); /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild)
        {
            goto fail;
        }
        if (next != NULL)
        {
            /* If newitem->child already set, then crosswire ->prev and ->next and move on */
            next->next = newchild;
            newchild->prev = next;
            next = newchild;
        }
        else
        {
            /* Set newitem->child and move to it */
            newitem->child = newchild;
            next = newchild;
        }
        child = child->next;
    }

    return newitem;

fail:
    if (newitem != NULL)
    {
        cJSON_Delete(newitem);
    }

    return NULL;
}

CJSON_PUBLIC(void) cJSON_Minify(char *json)
{
    unsigned char *into = (unsigned char*)json;
    while (*json)
    {
        if (*json == ' ')
        {
            json++;
        }
        else if (*json == '\t')
        {
            /* Whitespace characters. */
            json++;
        }
        else if (*json == '\r')
        {
            json++;
        }
        else if (*json=='\n')
        {
            json++;
        }
        else if ((*json == '/') && (json[1] == '/'))
        {
            /* double-slash comments, to end of line. */
            while (*json && (*json != '\n'))
            {
                json++;
            }
        }
        else if ((*json == '/') && (json[1] == '*'))
        {
            /* multiline comments. */
            while (*json && !((*json == '*') && (json[1] == '/')))
            {
                json++;
            }
            json += 2;
        }
        else if (*json == '\"')
        {
            /* string literals, which are \" sensitive. */
            *into++ = (unsigned char)*json++;
            while (*json && (*json != '\"'))
            {
                if (*json == '\\')
                {
                    *into++ = (unsigned char)*json++;
                }
                *into++ = (unsigned char)*json++;
            }
            *into++ = (unsigned char)*json++;
        }
        else
        {
            /* All other characters. */
            *into++ = (unsigned char)*json++;
        }
    }

    /* and null-terminate. */
    *into = '\0';
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsInvalid(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Invalid;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_False;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xff) == cJSON_True;
}


CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & (cJSON_True | cJSON_False)) != 0;
}
CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_NULL;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Number;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_String;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsArray(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Array;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsObject(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Object;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsRaw(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Raw;
}
