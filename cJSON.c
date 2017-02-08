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

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"

/* Determine the number of bits that an integer has using the preprocessor */
#if INT_MAX == 32767
    /* 16 bits */
    #define INTEGER_SIZE 0x0010
#elif INT_MAX == 2147483647
    /* 32 bits */
    #define INTEGER_SIZE 0x0100
#elif INT_MAX == 9223372036854775807
    /* 64 bits */
    #define INTEGER_SIZE 0x1000
#else
    #error "Failed to determine the size of an integer"
#endif

/* define our own boolean type */
typedef int cjbool;
#define true ((cjbool)1)
#define false ((cjbool)0)

static const unsigned char *global_ep = NULL;

const char *cJSON_GetErrorPtr(void)
{
    return (const char*) global_ep;
}

extern const char* cJSON_Version(void)
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
    for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)
    {
        if (*s1 == '\0')
        {
            return 0;
        }
    }

    return tolower(*s1) - tolower(*s2);
}

static void *(*cJSON_malloc)(size_t sz) = malloc;
static void (*cJSON_free)(void *ptr) = free;

static unsigned char* cJSON_strdup(const unsigned char* str)
{
    size_t len = 0;
    unsigned char *copy = NULL;

    if (str == NULL)
    {
        return NULL;
    }

    len = strlen((const char*)str) + 1;
    if (!(copy = (unsigned char*)cJSON_malloc(len)))
    {
        return NULL;
    }
    memcpy(copy, str, len);

    return copy;
}

void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (!hooks)
    {
        /* Reset hooks */
        cJSON_malloc = malloc;
        cJSON_free = free;
        return;
    }

    cJSON_malloc = (hooks->malloc_fn) ? hooks->malloc_fn : malloc;
    cJSON_free = (hooks->free_fn) ? hooks->free_fn : free;
}

/* Internal constructor. */
static cJSON *cJSON_New_Item(void)
{
    cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
    if (node)
    {
        memset(node, '\0', sizeof(cJSON));
    }

    return node;
}

/* Delete a cJSON structure. */
void cJSON_Delete(cJSON *c)
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
            cJSON_free(c->valuestring);
        }
        if (!(c->type & cJSON_StringIsConst) && c->string)
        {
            cJSON_free(c->string);
        }
        cJSON_free(c);
        c = next;
    }
}

/* Parse the input text to generate a number, and populate the result into item. */
static const unsigned char *parse_number(cJSON *item, const unsigned char *num)
{
    double number = 0;
    unsigned char *endpointer = NULL;

    if (num == NULL)
    {
        return NULL;
    }

    number = strtod((const char*)num, (char**)&endpointer);
    if ((num == endpointer) || (num == NULL))
    {
        /* parse_error */
        return NULL;
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

    return endpointer;
}

/* don't ask me, but the original cJSON_SetNumberValue returns an integer or double */
double cJSON_SetNumberHelper(cJSON *object, double number)
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

/* calculate the next largest power of 2 */
static int pow2gt (int x)
{
    --x;

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
#if INTEGER_SIZE & 0x1110 /* at least 16 bit */
    x |= x >> 8;
#endif
#if INTEGER_SIZE & 0x1100 /* at least 32 bit */
    x |= x >> 16;
#endif
#if INTEGER_SIZE & 0x1000 /* 64 bit */
    x |= x >> 32;
#endif

    return x + 1;
}

typedef struct
{
    unsigned char *buffer;
    size_t length;
    size_t offset;
    cjbool noalloc;
} printbuffer;

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static unsigned char* ensure(printbuffer *p, size_t needed)
{
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;

    if (needed > INT_MAX)
    {
        /* sizes bigger than INT_MAX are currently not supported */
        return NULL;
    }

    if (!p || !p->buffer)
    {
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

    newsize = (size_t) pow2gt((int)needed);
    newbuffer = (unsigned char*)cJSON_malloc(newsize);
    if (!newbuffer)
    {
        cJSON_free(p->buffer);
        p->length = 0;
        p->buffer = NULL;

        return NULL;
    }
    if (newbuffer)
    {
        memcpy(newbuffer, p->buffer, p->length);
    }
    cJSON_free(p->buffer);
    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

/* calculate the new length of the string in a printbuffer */
static size_t update(const printbuffer *p)
{
    const unsigned char *str = NULL;
    if (!p || !p->buffer)
    {
        return 0;
    }
    str = p->buffer + p->offset;

    return p->offset + strlen((const char*)str);
}

/* Render the number nicely from the given item into a string. */
static unsigned char *print_number(const cJSON *item, printbuffer *p)
{
    unsigned char *str = NULL;
    double d = item->valuedouble;
    /* special case for 0. */
    if (d == 0)
    {
        if (p)
        {
            str = ensure(p, 2);
        }
        else
        {
            str = (unsigned char*)cJSON_malloc(2);
        }
        if (str)
        {
            strcpy((char*)str,"0");
        }
    }
    /* value is an int */
    else if ((fabs(((double)item->valueint) - d) <= DBL_EPSILON) && (d <= INT_MAX) && (d >= INT_MIN))
    {
        if (p)
        {
            str = ensure(p, 21);
        }
        else
        {
            /* 2^64+1 can be represented in 21 chars. */
            str = (unsigned char*)cJSON_malloc(21);
        }
        if (str)
        {
            sprintf((char*)str, "%d", item->valueint);
        }
    }
    /* value is a floating point number */
    else
    {
        if (p)
        {
            /* This is a nice tradeoff. */
            str = ensure(p, 64);
        }
        else
        {
            /* This is a nice tradeoff. */
            str = (unsigned char*)cJSON_malloc(64);
        }
        if (str)
        {
            /* This checks for NaN and Infinity */
            if ((d * 0) != 0)
            {
                sprintf((char*)str, "null");
            }
            else if ((fabs(floor(d) - d) <= DBL_EPSILON) && (fabs(d) < 1.0e60))
            {
                sprintf((char*)str, "%.0f", d);
            }
            else if ((fabs(d) < 1.0e-6) || (fabs(d) > 1.0e9))
            {
                sprintf((char*)str, "%e", d);
            }
            else
            {
                sprintf((char*)str, "%f", d);
            }
        }
    }
    return str;
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const unsigned char *str)
{
    unsigned int h = 0;
    size_t i = 0;

    for (i = 0; i < 4; i++)
    {
        /* parse digit */
        if ((*str >= '0') && (*str <= '9'))
        {
            h += (unsigned int) (*str) - '0';
        }
        else if ((*str >= 'A') && (*str <= 'F'))
        {
            h += (unsigned int) 10 + (*str) - 'A';
        }
        else if ((*str >= 'a') && (*str <= 'f'))
        {
            h += (unsigned int) 10 + (*str) - 'a';
        }
        else /* invalid */
        {
            return 0;
        }

        if (i < 3)
        {
            /* shift left to make place for the next nibble */
            h = h << 4;
            str++;
        }
    }

    return h;
}

/* first bytes of UTF8 encoding for a given length in bytes */
static const unsigned char firstByteMark[5] =
{
    0x00, /* should never happen */
    0x00, /* 0xxxxxxx */
    0xC0, /* 110xxxxx */
    0xE0, /* 1110xxxx */
    0xF0 /* 11110xxx */
};

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char *parse_string(cJSON *item, const unsigned char *str, const unsigned char **ep)
{
    const unsigned char *ptr = str + 1;
    const unsigned char *end_ptr = str + 1;
    unsigned char *ptr2 = NULL;
    unsigned char *out = NULL;
    size_t len = 0;
    unsigned uc = 0;
    unsigned uc2 = 0;

    /* not a string! */
    if (*str != '\"')
    {
        *ep = str;
        goto fail;
    }

    while ((*end_ptr != '\"') && *end_ptr)
    {
        if (*end_ptr++ == '\\')
        {
            if (*end_ptr == '\0')
            {
                /* prevent buffer overflow when last input character is a backslash */
                goto fail;
            }
            /* Skip escaped quotes. */
            end_ptr++;
        }
        len++;
    }

    /* This is at most how long we need for the string, roughly. */
    out = (unsigned char*)cJSON_malloc(len + 1);
    if (!out)
    {
        goto fail;
    }
    item->valuestring = (char*)out; /* assign here so out will be deleted during cJSON_Delete() later */
    item->type = cJSON_String;

    ptr = str + 1;
    ptr2 = out;
    /* loop through the string literal */
    while (ptr < end_ptr)
    {
        if (*ptr != '\\')
        {
            *ptr2++ = *ptr++;
        }
        /* escape sequence */
        else
        {
            ptr++;
            switch (*ptr)
            {
                case 'b':
                    *ptr2++ = '\b';
                    break;
                case 'f':
                    *ptr2++ = '\f';
                    break;
                case 'n':
                    *ptr2++ = '\n';
                    break;
                case 'r':
                    *ptr2++ = '\r';
                    break;
                case 't':
                    *ptr2++ = '\t';
                    break;
                case '\"':
                case '\\':
                case '/':
                    *ptr2++ = *ptr;
                    break;
                case 'u':
                    /* transcode utf16 to utf8. See RFC2781 and RFC3629. */
                    uc = parse_hex4(ptr + 1); /* get the unicode char. */
                    ptr += 4;
                    if (ptr >= end_ptr)
                    {
                        /* invalid */
                        *ep = str;
                        goto fail;
                    }
                    /* check for invalid. */
                    if (((uc >= 0xDC00) && (uc <= 0xDFFF)) || (uc == 0))
                    {
                        *ep = str;
                        goto fail;
                    }

                    /* UTF16 surrogate pairs. */
                    if ((uc >= 0xD800) && (uc<=0xDBFF))
                    {
                        if ((ptr + 6) > end_ptr)
                        {
                            /* invalid */
                            *ep = str;
                            goto fail;
                        }
                        if ((ptr[1] != '\\') || (ptr[2] != 'u'))
                        {
                            /* missing second-half of surrogate. */
                            *ep = str;
                            goto fail;
                        }
                        uc2 = parse_hex4(ptr + 3);
                        ptr += 6; /* \uXXXX */
                        if ((uc2 < 0xDC00) || (uc2 > 0xDFFF))
                        {
                            /* invalid second-half of surrogate. */
                            *ep = str;
                            goto fail;
                        }
                        /* calculate unicode codepoint from the surrogate pair */
                        uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
                    }

                    /* encode as UTF8
                     * takes at maximum 4 bytes to encode:
                     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
                    len = 4;
                    if (uc < 0x80)
                    {
                        /* normal ascii, encoding 0xxxxxxx */
                        len = 1;
                    }
                    else if (uc < 0x800)
                    {
                        /* two bytes, encoding 110xxxxx 10xxxxxx */
                        len = 2;
                    }
                    else if (uc < 0x10000)
                    {
                        /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
                        len = 3;
                    }
                    ptr2 += len;

                    switch (len) {
                        case 4:
                            /* 10xxxxxx */
                            *--ptr2 = (unsigned char)((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 3:
                            /* 10xxxxxx */
                            *--ptr2 = (unsigned char)((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 2:
                            /* 10xxxxxx */
                            *--ptr2 = (unsigned char)((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 1:
                            /* depending on the length in bytes this determines the
                             * encoding ofthe first UTF8 byte */
                            *--ptr2 = (unsigned char)((uc | firstByteMark[len]) & 0xFF);
                            break;
                        default:
                            *ep = str;
                            goto fail;
                    }
                    ptr2 += len;
                    break;
                default:
                    *ep = str;
                    goto fail;
            }
            ptr++;
        }
    }
    *ptr2 = '\0';
    if (*ptr == '\"')
    {
        ptr++;
    }

    return ptr;

fail:
    if (out != NULL)
    {
        cJSON_free(out);
    }

    return NULL;
}

/* Render the cstring provided to an escaped version that can be printed. */
static unsigned char *print_string_ptr(const unsigned char *str, printbuffer *p)
{
    const unsigned char *ptr = NULL;
    unsigned char *ptr2 = NULL;
    unsigned char *out = NULL;
    size_t len = 0;
    cjbool flag = false;
    unsigned char token = '\0';

    /* empty string */
    if (!str)
    {
        if (p)
        {
            out = ensure(p, 3);
        }
        else
        {
            out = (unsigned char*)cJSON_malloc(3);
        }
        if (!out)
        {
            return NULL;
        }
        strcpy((char*)out, "\"\"");

        return out;
    }

    /* set "flag" to 1 if something needs to be escaped */
    for (ptr = str; *ptr; ptr++)
    {
        flag |= (((*ptr > 0) && (*ptr < 32)) /* unprintable characters */
                || (*ptr == '\"') /* double quote */
                || (*ptr == '\\')) /* backslash */
            ? 1
            : 0;
    }
    /* no characters have to be escaped */
    if (!flag)
    {
        len = (size_t)(ptr - str);
        if (p)
        {
            out = ensure(p, len + 3);
        }
        else
        {
            out = (unsigned char*)cJSON_malloc(len + 3);
        }
        if (!out)
        {
            return NULL;
        }

        ptr2 = out;
        *ptr2++ = '\"';
        strcpy((char*)ptr2, (const char*)str);
        ptr2[len] = '\"';
        ptr2[len + 1] = '\0';

        return out;
    }

    ptr = str;
    /* calculate additional space that is needed for escaping */
    while ((token = *ptr))
    {
        ++len;
        if (strchr("\"\\\b\f\n\r\t", token))
        {
            len++; /* +1 for the backslash */
        }
        else if (token < 32)
        {
            len += 5; /* +5 for \uXXXX */
        }
        ptr++;
    }

    if (p)
    {
        out = ensure(p, len + 3);
    }
    else
    {
        out = (unsigned char*)cJSON_malloc(len + 3);
    }
    if (!out)
    {
        return NULL;
    }

    ptr2 = out;
    ptr = str;
    *ptr2++ = '\"';
    /* copy the string */
    while (*ptr)
    {
        if ((*ptr > 31) && (*ptr != '\"') && (*ptr != '\\'))
        {
            /* normal character, copy */
            *ptr2++ = *ptr++;
        }
        else
        {
            /* character needs to be escaped */
            *ptr2++ = '\\';
            switch (token = *ptr++)
            {
                case '\\':
                    *ptr2++ = '\\';
                    break;
                case '\"':
                    *ptr2++ = '\"';
                    break;
                case '\b':
                    *ptr2++ = 'b';
                    break;
                case '\f':
                    *ptr2++ = 'f';
                    break;
                case '\n':
                    *ptr2++ = 'n';
                    break;
                case '\r':
                    *ptr2++ = 'r';
                    break;
                case '\t':
                    *ptr2++ = 't';
                    break;
                default:
                    /* escape and print as unicode codepoint */
                    sprintf((char*)ptr2, "u%04x", token);
                    ptr2 += 5;
                    break;
            }
        }
    }
    *ptr2++ = '\"';
    *ptr2++ = '\0';

    return out;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static unsigned char *print_string(const cJSON *item, printbuffer *p)
{
    return print_string_ptr((unsigned char*)item->valuestring, p);
}

/* Predeclare these prototypes. */
static const unsigned char *parse_value(cJSON *item, const unsigned char *value, const unsigned char **ep);
static unsigned char *print_value(const cJSON *item, size_t depth, cjbool fmt, printbuffer *p);
static const unsigned char *parse_array(cJSON *item, const unsigned char *value, const unsigned char **ep);
static unsigned char *print_array(const cJSON *item, size_t depth, cjbool fmt, printbuffer *p);
static const unsigned char *parse_object(cJSON *item, const unsigned char *value, const unsigned char **ep);
static unsigned char *print_object(const cJSON *item, size_t depth, cjbool fmt, printbuffer *p);

/* Utility to jump whitespace and cr/lf */
static const unsigned char *skip(const unsigned char *in)
{
    while (in && *in && (*in <= 32))
    {
        in++;
    }

    return in;
}

/* Parse an object - create a new root, and populate. */
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cjbool require_null_terminated)
{
    const unsigned char *end = NULL;
    /* use global error pointer if no specific one was given */
    const unsigned char **ep = return_parse_end ? (const unsigned char**)return_parse_end : &global_ep;
    cJSON *c = cJSON_New_Item();
    *ep = NULL;
    if (!c) /* memory fail */
    {
        return NULL;
    }

    end = parse_value(c, skip((const unsigned char*)value), ep);
    if (!end)
    {
        /* parse failure. ep is set. */
        cJSON_Delete(c);
        return NULL;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated)
    {
        end = skip(end);
        if (*end)
        {
            cJSON_Delete(c);
            *ep = end;
            return NULL;
        }
    }
    if (return_parse_end)
    {
        *return_parse_end = (const char*)end;
    }

    return c;
}

/* Default options for cJSON_Parse */
cJSON *cJSON_Parse(const char *value)
{
    return cJSON_ParseWithOpts(value, 0, 0);
}

/* Render a cJSON item/entity/structure to text. */
char *cJSON_Print(const cJSON *item)
{
    return (char*)print_value(item, 0, 1, 0);
}

char *cJSON_PrintUnformatted(const cJSON *item)
{
    return (char*)print_value(item, 0, 0, 0);
}

char *cJSON_PrintBuffered(const cJSON *item, int prebuffer, cjbool fmt)
{
    printbuffer p;

    if (prebuffer < 0)
    {
        return NULL;
    }

    p.buffer = (unsigned char*)cJSON_malloc((size_t)prebuffer);
    if (!p.buffer)
    {
        return NULL;
    }

    p.length = (size_t)prebuffer;
    p.offset = 0;
    p.noalloc = false;

    return (char*)print_value(item, 0, fmt, &p);
}

int cJSON_PrintPreallocated(cJSON *item, char *buf, const int len, const cjbool fmt)
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
    return print_value(item, 0, fmt, &p) != NULL;
}

/* Parser core - when encountering text, process appropriately. */
static const unsigned  char *parse_value(cJSON *item, const unsigned char *value, const unsigned char **ep)
{
    if (!value)
    {
        /* Fail on null. */
        return NULL;
    }

    /* parse the different types of values */
    if (!strncmp((const char*)value, "null", 4))
    {
        item->type = cJSON_NULL;
        return value + 4;
    }
    if (!strncmp((const char*)value, "false", 5))
    {
        item->type = cJSON_False;
        return value + 5;
    }
    if (!strncmp((const char*)value, "true", 4))
    {
        item->type = cJSON_True;
        item->valueint = 1;
        return value + 4;
    }
    if (*value == '\"')
    {
        return parse_string(item, value, ep);
    }
    if ((*value == '-') || ((*value >= '0') && (*value <= '9')))
    {
        return parse_number(item, value);
    }
    if (*value == '[')
    {
        return parse_array(item, value, ep);
    }
    if (*value == '{')
    {
        return parse_object(item, value, ep);
    }

    /* failure. */
    *ep = value;
    return NULL;
}

/* Render a value to text. */
static unsigned char *print_value(const cJSON *item, size_t depth, cjbool fmt, printbuffer *p)
{
    unsigned char *out = NULL;

    if (!item)
    {
        return NULL;
    }
    if (p)
    {
        switch ((item->type) & 0xFF)
        {
            case cJSON_NULL:
                out = ensure(p, 5);
                if (out)
                {
                    strcpy((char*)out, "null");
                }
                break;
            case cJSON_False:
                out = ensure(p, 6);
                if (out)
                {
                    strcpy((char*)out, "false");
                }
                break;
            case cJSON_True:
                out = ensure(p, 5);
                if (out)
                {
                    strcpy((char*)out, "true");
                }
                break;
            case cJSON_Number:
                out = print_number(item, p);
                break;
            case cJSON_Raw:
            {
                size_t raw_length = 0;
                if (item->valuestring == NULL)
                {
                    if (!p->noalloc)
                    {
                        cJSON_free(p->buffer);
                    }
                    out = NULL;
                    break;
                }

                raw_length = strlen(item->valuestring) + sizeof('\0');
                out = ensure(p, raw_length);
                if (out)
                {
                    memcpy(out, item->valuestring, raw_length);
                }
                break;
            }
            case cJSON_String:
                out = print_string(item, p);
                break;
            case cJSON_Array:
                out = print_array(item, depth, fmt, p);
                break;
            case cJSON_Object:
                out = print_object(item, depth, fmt, p);
                break;
            default:
                out = NULL;
                break;
        }
    }
    else
    {
        switch ((item->type) & 0xFF)
        {
            case cJSON_NULL:
                out = cJSON_strdup((const unsigned char*)"null");
                break;
            case cJSON_False:
                out = cJSON_strdup((const unsigned char*)"false");
                break;
            case cJSON_True:
                out = cJSON_strdup((const unsigned char*)"true");
                break;
            case cJSON_Number:
                out = print_number(item, 0);
                break;
            case cJSON_Raw:
                out = cJSON_strdup((unsigned char*)item->valuestring);
                break;
            case cJSON_String:
                out = print_string(item, 0);
                break;
            case cJSON_Array:
                out = print_array(item, depth, fmt, 0);
                break;
            case cJSON_Object:
                out = print_object(item, depth, fmt, 0);
                break;
            default:
                out = NULL;
                break;
        }
    }

    return out;
}

/* Build an array from input text. */
static const unsigned char *parse_array(cJSON *item, const unsigned char *value, const unsigned char **ep)
{
    cJSON *child = NULL;
    if (*value != '[')
    {
        /* not an array! */
        *ep = value;
        goto fail;
    }

    item->type = cJSON_Array;
    value = skip(value + 1);
    if (*value == ']')
    {
        /* empty array. */
        return value + 1;
    }

    item->child = child = cJSON_New_Item();
    if (!item->child)
    {
        /* memory fail */
        goto fail;
    }
    /* skip any spacing, get the value. */
    value = skip(parse_value(child, skip(value), ep));
    if (!value)
    {
        goto fail;
    }

    /* loop through the comma separated array elements */
    while (*value == ',')
    {
        cJSON *new_item = NULL;
        if (!(new_item = cJSON_New_Item()))
        {
            /* memory fail */
            goto fail;
        }
        /* add new item to end of the linked list */
        child->next = new_item;
        new_item->prev = child;
        child = new_item;

        /* go to the next comma */
        value = skip(parse_value(child, skip(value + 1), ep));
        if (!value)
        {
            /* memory fail */
            goto fail;
        }
    }

    if (*value == ']')
    {
        /* end of array */
        return value + 1;
    }

    /* malformed. */
    *ep = value;

fail:
    if (item->child != NULL)
    {
        cJSON_Delete(item->child);
        item->child = NULL;
    }

    return NULL;
}

/* Render an array to text */
static unsigned char *print_array(const cJSON *item, size_t depth, cjbool fmt, printbuffer *p)
{
    unsigned char **entries;
    unsigned char *out = NULL;
    unsigned char *ptr = NULL;
    unsigned char *ret = NULL;
    size_t len = 5;
    cJSON *child = item->child;
    size_t numentries = 0;
    size_t i = 0;
    cjbool fail = false;
    size_t tmplen = 0;

    /* How many entries in the array? */
    while (child)
    {
        numentries++;
        child = child->next;
    }

    /* Explicitly handle numentries == 0 */
    if (!numentries)
    {
        if (p)
        {
            out = ensure(p, 3);
        }
        else
        {
            out = (unsigned char*)cJSON_malloc(3);
        }
        if (out)
        {
            strcpy((char*)out, "[]");
        }

        return out;
    }

    if (p)
    {
        /* Compose the output array. */
        /* opening square bracket */
        i = p->offset;
        ptr = ensure(p, 1);
        if (!ptr)
        {
            return NULL;
        }
        *ptr = '[';
        p->offset++;

        child = item->child;
        while (child && !fail)
        {
            if (!print_value(child, depth + 1, fmt, p))
            {
                return NULL;
            }
            p->offset = update(p);
            if (child->next)
            {
                len = fmt ? 2 : 1;
                ptr = ensure(p, len + 1);
                if (!ptr)
                {
                    return NULL;
                }
                *ptr++ = ',';
                if(fmt)
                {
                    *ptr++ = ' ';
                }
                *ptr = '\0';
                p->offset += len;
            }
            child = child->next;
        }
        ptr = ensure(p, 2);
        if (!ptr)
        {
            return NULL;
        }
        *ptr++ = ']';
        *ptr = '\0';
        out = (p->buffer) + i;
    }
    else
    {
        /* Allocate an array to hold the pointers to all printed values */
        entries = (unsigned char**)cJSON_malloc(numentries * sizeof(unsigned char*));
        if (!entries)
        {
            return NULL;
        }
        memset(entries, '\0', numentries * sizeof(unsigned char*));

        /* Retrieve all the results: */
        child = item->child;
        while (child && !fail)
        {
            ret = print_value(child, depth + 1, fmt, 0);
            entries[i++] = ret;
            if (ret)
            {
                len += strlen((char*)ret) + 2 + (fmt ? 1 : 0);
            }
            else
            {
                fail = true;
            }
            child = child->next;
        }

        /* If we didn't fail, try to malloc the output string */
        if (!fail)
        {
            out = (unsigned char*)cJSON_malloc(len);
        }
        /* If that fails, we fail. */
        if (!out)
        {
            fail = true;
        }

        /* Handle failure. */
        if (fail)
        {
            /* free all the entries in the array */
            for (i = 0; i < numentries; i++)
            {
                if (entries[i])
                {
                    cJSON_free(entries[i]);
                }
            }
            cJSON_free(entries);
            return NULL;
        }

        /* Compose the output array. */
        *out='[';
        ptr = out + 1;
        *ptr = '\0';
        for (i = 0; i < numentries; i++)
        {
            tmplen = strlen((char*)entries[i]);
            memcpy(ptr, entries[i], tmplen);
            ptr += tmplen;
            if (i != (numentries - 1))
            {
                *ptr++ = ',';
                if(fmt)
                {
                    *ptr++ = ' ';
                }
                *ptr = '\0';
            }
            cJSON_free(entries[i]);
        }
        cJSON_free(entries);
        *ptr++ = ']';
        *ptr++ = '\0';
    }

    return out;
}

/* Build an object from the text. */
static const unsigned char *parse_object(cJSON *item, const unsigned char *value, const unsigned char **ep)
{
    cJSON *child = NULL;
    if (*value != '{')
    {
        /* not an object! */
        *ep = value;
        goto fail;
    }

    item->type = cJSON_Object;
    value = skip(value + 1);
    if (*value == '}')
    {
        /* empty object. */
        return value + 1;
    }

    child = cJSON_New_Item();
    item->child = child;
    if (!item->child)
    {
        goto fail;
    }
    /* parse first key */
    value = skip(parse_string(child, skip(value), ep));
    if (!value)
    {
        goto fail;
    }
    /* use string as key, not value */
    child->string = child->valuestring;
    child->valuestring = NULL;

    if (*value != ':')
    {
        /* invalid object. */
        *ep = value;
        goto fail;
    }
    /* skip any spacing, get the value. */
    value = skip(parse_value(child, skip(value + 1), ep));
    if (!value)
    {
        goto fail;
    }

    while (*value == ',')
    {
        cJSON *new_item = NULL;
        if (!(new_item = cJSON_New_Item()))
        {
            /* memory fail */
            goto fail;
        }
        /* add to linked list */
        child->next = new_item;
        new_item->prev = child;

        child = new_item;
        value = skip(parse_string(child, skip(value + 1), ep));
        if (!value)
        {
            goto fail;
        }

        /* use string as key, not value */
        child->string = child->valuestring;
        child->valuestring = NULL;

        if (*value != ':')
        {
            /* invalid object. */
            *ep = value;
            goto fail;
        }
        /* skip any spacing, get the value. */
        value = skip(parse_value(child, skip(value + 1), ep));
        if (!value)
        {
            goto fail;
        }
    }
    /* end of object */
    if (*value == '}')
    {
        return value + 1;
    }

    /* malformed */
    *ep = value;

fail:
    if (item->child != NULL)
    {
        cJSON_Delete(child);
        item->child = NULL;
    }

    return NULL;
}

/* Render an object to text. */
static unsigned char *print_object(const cJSON *item, size_t depth, cjbool fmt, printbuffer *p)
{
    unsigned char **entries = NULL;
    unsigned char **names = NULL;
    unsigned char *out = NULL;
    unsigned char *ptr = NULL;
    unsigned char *ret = NULL;
    unsigned char *str = NULL;
    size_t len = 7;
    size_t i = 0;
    size_t j = 0;
    cJSON *child = item->child;
    size_t numentries = 0;
    cjbool fail = false;
    size_t tmplen = 0;

    /* Count the number of entries. */
    while (child)
    {
        numentries++;
        child = child->next;
    }

    /* Explicitly handle empty object case */
    if (!numentries)
    {
        if (p)
        {
            out = ensure(p, fmt ? depth + 4 : 3);
        }
        else
        {
            out = (unsigned char*)cJSON_malloc(fmt ? depth + 4 : 3);
        }
        if (!out)
        {
            return NULL;
        }
        ptr = out;
        *ptr++ = '{';
        if (fmt) {
            *ptr++ = '\n';
            for (i = 0; i < depth; i++)
            {
                *ptr++ = '\t';
            }
        }
        *ptr++ = '}';
        *ptr++ = '\0';

        return out;
    }

    if (p)
    {
        /* Compose the output: */
        i = p->offset;
        len = fmt ? 2 : 1; /* fmt: {\n */
        ptr = ensure(p, len + 1);
        if (!ptr)
        {
            return NULL;
        }

        *ptr++ = '{';
        if (fmt)
        {
            *ptr++ = '\n';
        }
        *ptr = '\0';
        p->offset += len;

        child = item->child;
        depth++;
        while (child)
        {
            if (fmt)
            {
                ptr = ensure(p, depth);
                if (!ptr)
                {
                    return NULL;
                }
                for (j = 0; j < depth; j++)
                {
                    *ptr++ = '\t';
                }
                p->offset += depth;
            }

            /* print key */
            if (!print_string_ptr((unsigned char*)child->string, p))
            {
                return NULL;
            }
            p->offset = update(p);

            len = fmt ? 2 : 1;
            ptr = ensure(p, len);
            if (!ptr)
            {
                return NULL;
            }
            *ptr++ = ':';
            if (fmt)
            {
                *ptr++ = '\t';
            }
            p->offset+=len;

            /* print value */
            if (!print_value(child, depth, fmt, p))
            {
                return NULL;
            };
            p->offset = update(p);

            /* print comma if not last */
            len = (size_t) (fmt ? 1 : 0) + (child->next ? 1 : 0);
            ptr = ensure(p, len + 1);
            if (!ptr)
            {
                return NULL;
            }
            if (child->next)
            {
                *ptr++ = ',';
            }

            if (fmt)
            {
                *ptr++ = '\n';
            }
            *ptr = '\0';
            p->offset += len;

            child = child->next;
        }

        ptr = ensure(p, fmt ? (depth + 1) : 2);
        if (!ptr)
        {
            return NULL;
        }
        if (fmt)
        {
            for (i = 0; i < (depth - 1); i++)
            {
                *ptr++ = '\t';
            }
        }
        *ptr++ = '}';
        *ptr = '\0';
        out = (p->buffer) + i;
    }
    else
    {
        /* Allocate space for the names and the objects */
        entries = (unsigned char**)cJSON_malloc(numentries * sizeof(unsigned char*));
        if (!entries)
        {
            return NULL;
        }
        names = (unsigned char**)cJSON_malloc(numentries * sizeof(unsigned char*));
        if (!names)
        {
            cJSON_free(entries);
            return NULL;
        }
        memset(entries, '\0', sizeof(unsigned char*) * numentries);
        memset(names, '\0', sizeof(unsigned char*) * numentries);

        /* Collect all the results into our arrays: */
        child = item->child;
        depth++;
        if (fmt)
        {
            len += depth;
        }
        while (child && !fail)
        {
            names[i] = str = print_string_ptr((unsigned char*)child->string, 0); /* print key */
            entries[i++] = ret = print_value(child, depth, fmt, 0);
            if (str && ret)
            {
                len += strlen((char*)ret) + strlen((char*)str) + 2 + (fmt ? 2 + depth : 0);
            }
            else
            {
                fail = true;
            }
            child = child->next;
        }

        /* Try to allocate the output string */
        if (!fail)
        {
            out = (unsigned char*)cJSON_malloc(len);
        }
        if (!out)
        {
            fail = true;
        }

        /* Handle failure */
        if (fail)
        {
            /* free all the printed keys and values */
            for (i = 0; i < numentries; i++)
            {
                if (names[i])
                {
                    cJSON_free(names[i]);
                }
                if (entries[i])
                {
                    cJSON_free(entries[i]);
                }
            }
            cJSON_free(names);
            cJSON_free(entries);
            return NULL;
        }

        /* Compose the output: */
        *out = '{';
        ptr = out + 1;
        if (fmt)
        {
            *ptr++ = '\n';
        }
        *ptr = '\0';
        for (i = 0; i < numentries; i++)
        {
            if (fmt)
            {
                for (j = 0; j < depth; j++)
                {
                    *ptr++='\t';
                }
            }
            tmplen = strlen((char*)names[i]);
            memcpy(ptr, names[i], tmplen);
            ptr += tmplen;
            *ptr++ = ':';
            if (fmt)
            {
                *ptr++ = '\t';
            }
            strcpy((char*)ptr, (char*)entries[i]);
            ptr += strlen((char*)entries[i]);
            if (i != (numentries - 1))
            {
                *ptr++ = ',';
            }
            if (fmt)
            {
                *ptr++ = '\n';
            }
            *ptr = '\0';
            cJSON_free(names[i]);
            cJSON_free(entries[i]);
        }

        cJSON_free(names);
        cJSON_free(entries);
        if (fmt)
        {
            for (i = 0; i < (depth - 1); i++)
            {
                *ptr++ = '\t';
            }
        }
        *ptr++ = '}';
        *ptr++ = '\0';
    }

    return out;
}

/* Get Array size/item / object item. */
int cJSON_GetArraySize(const cJSON *array)
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

cJSON *cJSON_GetArrayItem(const cJSON *array, int item)
{
    cJSON *c = array ? array->child : NULL;
    while (c && item > 0)
    {
        item--;
        c = c->next;
    }

    return c;
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string)
{
    cJSON *c = object ? object->child : NULL;
    while (c && cJSON_strcasecmp((unsigned char*)c->string, (const unsigned char*)string))
    {
        c = c->next;
    }
    return c;
}

cjbool cJSON_HasObjectItem(const cJSON *object, const char *string)
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
static cJSON *create_reference(const cJSON *item)
{
    cJSON *ref = cJSON_New_Item();
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
void cJSON_AddItemToArray(cJSON *array, cJSON *item)
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

void   cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    if (!item)
    {
        return;
    }

    /* free old key and set new one */
    if (item->string)
    {
        cJSON_free(item->string);
    }
    item->string = (char*)cJSON_strdup((const unsigned char*)string);

    cJSON_AddItemToArray(object,item);
}

/* Add an item to an object with constant string as key */
void   cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item)
{
    if (!item)
    {
        return;
    }
    if (!(item->type & cJSON_StringIsConst) && item->string)
    {
        cJSON_free(item->string);
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    item->string = (char*)string;
#pragma GCC diagnostic pop
    item->type |= cJSON_StringIsConst;
    cJSON_AddItemToArray(object, item);
}

void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)
{
    cJSON_AddItemToArray(array, create_reference(item));
}

void cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item)
{
    cJSON_AddItemToObject(object, string, create_reference(item));
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
cJSON *cJSON_DetachItemFromArray(cJSON *array, int which)
{
    if (which < 0)
    {
        return NULL;
    }

    return DetachItemFromArray(array, (size_t)which);
}

void cJSON_DeleteItemFromArray(cJSON *array, int which)
{
    cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}

cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string)
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

void cJSON_DeleteItemFromObject(cJSON *object, const char *string)
{
    cJSON_Delete(cJSON_DetachItemFromObject(object, string));
}

/* Replace array/object items with new ones. */
void cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem)
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
void cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem)
{
    if (which < 0)
    {
        return;
    }

    ReplaceItemInArray(array, (size_t)which, newitem);
}

void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem)
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
             cJSON_free(newitem->string);
        }

        newitem->string = (char*)cJSON_strdup((const unsigned char*)string);
        ReplaceItemInArray(object, i, newitem);
    }
}

/* Create basic types: */
cJSON *cJSON_CreateNull(void)
{
    cJSON *item = cJSON_New_Item();
    if(item)
    {
        item->type = cJSON_NULL;
    }

    return item;
}

cJSON *cJSON_CreateTrue(void)
{
    cJSON *item = cJSON_New_Item();
    if(item)
    {
        item->type = cJSON_True;
    }

    return item;
}

cJSON *cJSON_CreateFalse(void)
{
    cJSON *item = cJSON_New_Item();
    if(item)
    {
        item->type = cJSON_False;
    }

    return item;
}

cJSON *cJSON_CreateBool(cjbool b)
{
    cJSON *item = cJSON_New_Item();
    if(item)
    {
        item->type = b ? cJSON_True : cJSON_False;
    }

    return item;
}

cJSON *cJSON_CreateNumber(double num)
{
    cJSON *item = cJSON_New_Item();
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

cJSON *cJSON_CreateString(const char *string)
{
    cJSON *item = cJSON_New_Item();
    if(item)
    {
        item->type = cJSON_String;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)string);
        if(!item->valuestring)
        {
            cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

extern cJSON *cJSON_CreateRaw(const char *raw)
{
    cJSON *item = cJSON_New_Item();
    if(item)
    {
        item->type = cJSON_Raw;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)raw);
        if(!item->valuestring)
        {
            cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

cJSON *cJSON_CreateArray(void)
{
    cJSON *item = cJSON_New_Item();
    if(item)
    {
        item->type=cJSON_Array;
    }

    return item;
}

cJSON *cJSON_CreateObject(void)
{
    cJSON *item = cJSON_New_Item();
    if (item)
    {
        item->type = cJSON_Object;
    }

    return item;
}

/* Create Arrays: */
cJSON *cJSON_CreateIntArray(const int *numbers, int count)
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

cJSON *cJSON_CreateFloatArray(const float *numbers, int count)
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

cJSON *cJSON_CreateDoubleArray(const double *numbers, int count)
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

cJSON *cJSON_CreateStringArray(const char **strings, int count)
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
cJSON *cJSON_Duplicate(const cJSON *item, cjbool recurse)
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
    newitem = cJSON_New_Item();
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
        newitem->valuestring = (char*)cJSON_strdup((unsigned char*)item->valuestring);
        if (!newitem->valuestring)
        {
            goto fail;
        }
    }
    if (item->string)
    {
        newitem->string = (item->type&cJSON_StringIsConst) ? item->string : (char*)cJSON_strdup((unsigned char*)item->string);
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

void cJSON_Minify(char *json)
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
