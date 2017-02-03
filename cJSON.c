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

static const char *global_ep = NULL;

const char *cJSON_GetErrorPtr(void)
{
    return global_ep;
}

extern const char* cJSON_Version(void)
{
    static char version[15];
    sprintf(version, "%i.%i.%i", CJSON_VERSION_MAJOR, CJSON_VERSION_MINOR, CJSON_VERSION_PATCH);

    return version;
}

/* case insensitive strcmp */
static int cJSON_strcasecmp(const char *s1, const char *s2)
{
    if (!s1)
    {
        return (s1 == s2) ? 0 : 1; /* both NULL? */
    }
    if (!s2)
    {
        return 1;
    }
    for(; tolower(*(const unsigned char *)s1) == tolower(*(const unsigned char *)s2); ++s1, ++s2)
    {
        if (*s1 == '\0')
        {
            return 0;
        }
    }

    return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*cJSON_malloc)(size_t sz) = malloc;
static void (*cJSON_free)(void *ptr) = free;

static char* cJSON_strdup(const char* str)
{
    size_t len = 0;
    char *copy = NULL;

    if (str == NULL)
    {
        return NULL;
    }

    len = strlen(str) + 1;
    if (!(copy = (char*)cJSON_malloc(len)))
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
static const char *parse_number(cJSON *item, const char *num)
{
    double n = 0;
    double sign = 1;
    double scale = 0;
    int subscale = 0;
    int signsubscale = 1;

    /* Has sign? */
    if (*num == '-')
    {
        sign = -1;
        num++;
    }
    /* is zero */
    if (*num == '0')
    {
        num++;
    }
    /* Number? */
    if ((*num >= '1') && (*num <= '9'))
    {
        do
        {
            n = (n * 10.0) + (*num++ - '0');
        }
        while ((*num >= '0') && (*num<='9'));
    }
    /* Fractional part? */
    if ((*num == '.') && (num[1] >= '0') && (num[1] <= '9'))
    {
        num++;
        do
        {
            n = (n  *10.0) + (*num++ - '0');
            scale--;
        } while ((*num >= '0') && (*num <= '9'));
    }
    /* Exponent? */
    if ((*num == 'e') || (*num == 'E'))
    {
        num++;
        /* With sign? */
        if (*num == '+')
        {
            num++;
        }
        else if (*num == '-')
        {
            signsubscale = -1;
            num++;
        }
        /* Number? */
        while ((*num>='0') && (*num<='9'))
        {
            subscale = (subscale * 10) + (*num++ - '0');
        }
    }

    /* number = +/- number.fraction * 10^+/- exponent */
    n = sign * n * pow(10.0, (scale + subscale * signsubscale));

    item->valuedouble = n;
    item->valueint = (int)n;
    item->type = cJSON_Number;

    return num;
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
    char *buffer;
    int length;
    int offset;
    cjbool noalloc;
} printbuffer;

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static char* ensure(printbuffer *p, int needed)
{
    char *newbuffer = NULL;
    int newsize = 0;
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

    newsize = pow2gt(needed);
    newbuffer = (char*)cJSON_malloc(newsize);
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
static int update(const printbuffer *p)
{
    char *str = NULL;
    if (!p || !p->buffer)
    {
        return 0;
    }
    str = p->buffer + p->offset;

    return p->offset + strlen(str);
}

/* Render the number nicely from the given item into a string. */
static char *print_number(const cJSON *item, printbuffer *p)
{
    char *str = NULL;
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
            str = (char*)cJSON_malloc(2);
        }
        if (str)
        {
            strcpy(str,"0");
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
            str = (char*)cJSON_malloc(21);
        }
        if (str)
        {
            sprintf(str, "%d", item->valueint);
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
            str=(char*)cJSON_malloc(64);
        }
        if (str)
        {
            /* This checks for NaN and Infinity */
            if ((d * 0) != 0)
            {
                sprintf(str, "null");
            }
            else if ((fabs(floor(d) - d) <= DBL_EPSILON) && (fabs(d) < 1.0e60))
            {
                sprintf(str, "%.0f", d);
            }
            else if ((fabs(d) < 1.0e-6) || (fabs(d) > 1.0e9))
            {
                sprintf(str, "%e", d);
            }
            else
            {
                sprintf(str, "%f", d);
            }
        }
    }
    return str;
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const char *str)
{
    unsigned h = 0;
    /* first digit */
    if ((*str >= '0') && (*str <= '9'))
    {
        h += (*str) - '0';
    }
    else if ((*str >= 'A') && (*str <= 'F'))
    {
        h += 10 + (*str) - 'A';
    }
    else if ((*str >= 'a') && (*str <= 'f'))
    {
        h += 10 + (*str) - 'a';
    }
    else /* invalid */
    {
        return 0;
    }


    /* second digit */
    h = h << 4;
    str++;
    if ((*str >= '0') && (*str <= '9'))
    {
        h += (*str) - '0';
    }
    else if ((*str >= 'A') && (*str <= 'F'))
    {
        h += 10 + (*str) - 'A';
    }
    else if ((*str >= 'a') && (*str <= 'f'))
    {
        h += 10 + (*str) - 'a';
    }
    else /* invalid */
    {
        return 0;
    }

    /* third digit */
    h = h << 4;
    str++;
    if ((*str >= '0') && (*str <= '9'))
    {
        h += (*str) - '0';
    }
    else if ((*str >= 'A') && (*str <= 'F'))
    {
        h += 10 + (*str) - 'A';
    }
    else if ((*str >= 'a') && (*str <= 'f'))
    {
        h += 10 + (*str) - 'a';
    }
    else /* invalid */
    {
        return 0;
    }

    /* fourth digit */
    h = h << 4;
    str++;
    if ((*str >= '0') && (*str <= '9'))
    {
        h += (*str) - '0';
    }
    else if ((*str >= 'A') && (*str <= 'F'))
    {
        h += 10 + (*str) - 'A';
    }
    else if ((*str >= 'a') && (*str <= 'f'))
    {
        h += 10 + (*str) - 'a';
    }
    else /* invalid */
    {
        return 0;
    }

    return h;
}

/* first bytes of UTF8 encoding for a given length in bytes */
static const unsigned char firstByteMark[7] =
{
    0x00, /* should never happen */
    0x00, /* 0xxxxxxx */
    0xC0, /* 110xxxxx */
    0xE0, /* 1110xxxx */
    0xF0, /* 11110xxx */
    0xF8,
    0xFC
};

/* Parse the input text into an unescaped cstring, and populate item. */
static const char *parse_string(cJSON *item, const char *str, const char **ep)
{
    const char *ptr = str + 1;
    const char *end_ptr =str + 1;
    char *ptr2 = NULL;
    char *out = NULL;
    int len = 0;
    unsigned uc = 0;
    unsigned uc2 = 0;

    /* not a string! */
    if (*str != '\"')
    {
        *ep = str;
        return NULL;
    }

    while ((*end_ptr != '\"') && *end_ptr)
    {
        if (*end_ptr++ == '\\')
        {
            if (*end_ptr == '\0')
            {
                /* prevent buffer overflow when last input character is a backslash */
                return NULL;
            }
            /* Skip escaped quotes. */
            end_ptr++;
        }
        len++;
    }

    /* This is at most how long we need for the string, roughly. */
    out = (char*)cJSON_malloc(len + 1);
    if (!out)
    {
        return NULL;
    }
    item->valuestring = out; /* assign here so out will be deleted during cJSON_Delete() later */
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
                        return NULL;
                    }
                    /* check for invalid. */
                    if (((uc >= 0xDC00) && (uc <= 0xDFFF)) || (uc == 0))
                    {
                        *ep = str;
                        return NULL;
                    }

                    /* UTF16 surrogate pairs. */
                    if ((uc >= 0xD800) && (uc<=0xDBFF))
                    {
                        if ((ptr + 6) > end_ptr)
                        {
                            /* invalid */
                            *ep = str;
                            return NULL;
                        }
                        if ((ptr[1] != '\\') || (ptr[2] != 'u'))
                        {
                            /* missing second-half of surrogate. */
                            *ep = str;
                            return NULL;
                        }
                        uc2 = parse_hex4(ptr + 3);
                        ptr += 6; /* \uXXXX */
                        if ((uc2 < 0xDC00) || (uc2 > 0xDFFF))
                        {
                            /* invalid second-half of surrogate. */
                            *ep = str;
                            return NULL;
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
                            *--ptr2 = ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 3:
                            /* 10xxxxxx */
                            *--ptr2 = ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 2:
                            /* 10xxxxxx */
                            *--ptr2 = ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 1:
                            /* depending on the length in bytes this determines the
                             * encoding ofthe first UTF8 byte */
                            *--ptr2 = (uc | firstByteMark[len]);
                    }
                    ptr2 += len;
                    break;
                default:
                    *ep = str;
                    return NULL;
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
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str, printbuffer *p)
{
    const char *ptr = NULL;
    char *ptr2 = NULL;
    char *out = NULL;
    int len = 0;
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
            out = (char*)cJSON_malloc(3);
        }
        if (!out)
        {
            return NULL;
        }
        strcpy(out, "\"\"");

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
        len = ptr - str;
        if (p)
        {
            out = ensure(p, len + 3);
        }
        else
        {
            out = (char*)cJSON_malloc(len + 3);
        }
        if (!out)
        {
            return NULL;
        }

        ptr2 = out;
        *ptr2++ = '\"';
        strcpy(ptr2, str);
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
        out = (char*)cJSON_malloc(len + 3);
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
        if (((unsigned char)*ptr > 31) && (*ptr != '\"') && (*ptr != '\\'))
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
                    sprintf(ptr2, "u%04x", token);
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
static char *print_string(const cJSON *item, printbuffer *p)
{
    return print_string_ptr(item->valuestring, p);
}

/* Predeclare these prototypes. */
static const char *parse_value(cJSON *item, const char *value, const char **ep);
static char *print_value(const cJSON *item, int depth, cjbool fmt, printbuffer *p);
static const char *parse_array(cJSON *item, const char *value, const char **ep);
static char *print_array(const cJSON *item, int depth, cjbool fmt, printbuffer *p);
static const char *parse_object(cJSON *item, const char *value, const char **ep);
static char *print_object(const cJSON *item, int depth, cjbool fmt, printbuffer *p);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in)
{
    while (in && *in && ((unsigned char)*in<=32))
    {
        in++;
    }

    return in;
}

/* Parse an object - create a new root, and populate. */
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cjbool require_null_terminated)
{
    const char *end = NULL;
    /* use global error pointer if no specific one was given */
    const char **ep = return_parse_end ? return_parse_end : &global_ep;
    cJSON *c = cJSON_New_Item();
    *ep = NULL;
    if (!c) /* memory fail */
    {
        return NULL;
    }

    end = parse_value(c, skip(value), ep);
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
        *return_parse_end = end;
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
    return print_value(item, 0, 1, 0);
}

char *cJSON_PrintUnformatted(const cJSON *item)
{
    return print_value(item, 0, 0, 0);
}

char *cJSON_PrintBuffered(const cJSON *item, int prebuffer, cjbool fmt)
{
    printbuffer p;
    p.buffer = (char*)cJSON_malloc(prebuffer);
    if (!p.buffer)
    {
        return NULL;
    }
    p.length = prebuffer;
    p.offset = 0;
    p.noalloc = false;

    return print_value(item, 0, fmt, &p);
}

int cJSON_PrintPreallocated(cJSON *item,char *buf, const int len, const cjbool fmt)
{
    printbuffer p;
    p.buffer = buf;
    p.length = len;
    p.offset = 0;
    p.noalloc = true;
    return print_value(item,0,fmt,&p) != NULL;
}

/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(cJSON *item, const char *value, const char **ep)
{
    if (!value)
    {
        /* Fail on null. */
        return NULL;
    }

    /* parse the different types of values */
    if (!strncmp(value, "null", 4))
    {
        item->type = cJSON_NULL;
        return value + 4;
    }
    if (!strncmp(value, "false", 5))
    {
        item->type = cJSON_False;
        return value + 5;
    }
    if (!strncmp(value, "true", 4))
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
static char *print_value(const cJSON *item, int depth, cjbool fmt, printbuffer *p)
{
    char *out = NULL;

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
                    strcpy(out, "null");
                }
                break;
            case cJSON_False:
                out = ensure(p, 6);
                if (out)
                {
                    strcpy(out, "false");
                }
                break;
            case cJSON_True:
                out = ensure(p, 5);
                if (out)
                {
                    strcpy(out, "true");
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
        }
    }
    else
    {
        switch ((item->type) & 0xFF)
        {
            case cJSON_NULL:
                out = cJSON_strdup("null");
                break;
            case cJSON_False:
                out = cJSON_strdup("false");
                break;
            case cJSON_True:
                out = cJSON_strdup("true");
                break;
            case cJSON_Number:
                out = print_number(item, 0);
                break;
            case cJSON_Raw:
                out = cJSON_strdup(item->valuestring);
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
        }
    }

    return out;
}

/* Build an array from input text. */
static const char *parse_array(cJSON *item,const char *value,const char **ep)
{
    cJSON *child = NULL;
    if (*value != '[')
    {
        /* not an array! */
        *ep = value;
        return NULL;
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
        return NULL;
    }
    /* skip any spacing, get the value. */
    value = skip(parse_value(child, skip(value), ep));
    if (!value)
    {
        return NULL;
    }

    /* loop through the comma separated array elements */
    while (*value == ',')
    {
        cJSON *new_item = NULL;
        if (!(new_item = cJSON_New_Item()))
        {
            /* memory fail */
            return NULL;
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
            return NULL;
        }
    }

    if (*value == ']')
    {
        /* end of array */
        return value + 1;
    }

    /* malformed. */
    *ep = value;

    return NULL;
}

/* Render an array to text */
static char *print_array(const cJSON *item, int depth, cjbool fmt, printbuffer *p)
{
    char **entries;
    char *out = NULL;
    char *ptr = NULL;
    char *ret = NULL;
    int len = 5;
    cJSON *child = item->child;
    int numentries = 0;
    int i = 0;
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
            out = (char*)cJSON_malloc(3);
        }
        if (out)
        {
            strcpy(out,"[]");
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
        entries = (char**)cJSON_malloc(numentries * sizeof(char*));
        if (!entries)
        {
            return NULL;
        }
        memset(entries, '\0', numentries * sizeof(char*));

        /* Retrieve all the results: */
        child = item->child;
        while (child && !fail)
        {
            ret = print_value(child, depth + 1, fmt, 0);
            entries[i++] = ret;
            if (ret)
            {
                len += strlen(ret) + 2 + (fmt ? 1 : 0);
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
            out = (char*)cJSON_malloc(len);
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
            tmplen = strlen(entries[i]);
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
static const char *parse_object(cJSON *item, const char *value, const char **ep)
{
    cJSON *child = NULL;
    if (*value != '{')
    {
        /* not an object! */
        *ep = value;
        return NULL;
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
        return NULL;
    }
    /* parse first key */
    value = skip(parse_string(child, skip(value), ep));
    if (!value)
    {
        return NULL;
    }
    /* use string as key, not value */
    child->string = child->valuestring;
    child->valuestring = NULL;

    if (*value != ':')
    {
        /* invalid object. */
        *ep = value;
        return NULL;
    }
    /* skip any spacing, get the value. */
    value = skip(parse_value(child, skip(value + 1), ep));
    if (!value)
    {
        return NULL;
    }

    while (*value == ',')
    {
        cJSON *new_item = NULL;
        if (!(new_item = cJSON_New_Item()))
        {
            /* memory fail */
            return NULL;
        }
        /* add to linked list */
        child->next = new_item;
        new_item->prev = child;

        child = new_item;
        value = skip(parse_string(child, skip(value + 1), ep));
        if (!value)
        {
            return NULL;
        }

        /* use string as key, not value */
        child->string = child->valuestring;
        child->valuestring = NULL;

        if (*value != ':')
        {
            /* invalid object. */
            *ep = value;
            return NULL;
        }
        /* skip any spacing, get the value. */
        value = skip(parse_value(child, skip(value + 1), ep));
        if (!value)
        {
            return NULL;
        }
    }
    /* end of object */
    if (*value == '}')
    {
        return value + 1;
    }

    /* malformed */
    *ep = value;
    return NULL;
}

/* Render an object to text. */
static char *print_object(const cJSON *item, int depth, cjbool fmt, printbuffer *p)
{
    char **entries = NULL;
    char **names = NULL;
    char *out = NULL;
    char *ptr = NULL;
    char *ret = NULL;
    char *str = NULL;
    int len = 7;
    int i = 0;
    int j = 0;
    cJSON *child = item->child;
    int numentries = 0;
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
            out = (char*)cJSON_malloc(fmt ? depth + 4 : 3);
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
            if (!print_string_ptr(child->string, p))
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
            len = (fmt ? 1 : 0) + (child->next ? 1 : 0);
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
        entries = (char**)cJSON_malloc(numentries * sizeof(char*));
        if (!entries)
        {
            return NULL;
        }
        names = (char**)cJSON_malloc(numentries * sizeof(char*));
        if (!names)
        {
            cJSON_free(entries);
            return NULL;
        }
        memset(entries, '\0', sizeof(char*) * numentries);
        memset(names, '\0', sizeof(char*) * numentries);

        /* Collect all the results into our arrays: */
        child = item->child;
        depth++;
        if (fmt)
        {
            len += depth;
        }
        while (child && !fail)
        {
            names[i] = str = print_string_ptr(child->string, 0); /* print key */
            entries[i++] = ret = print_value(child, depth, fmt, 0);
            if (str && ret)
            {
                len += strlen(ret) + strlen(str) + 2 + (fmt ? 2 + depth : 0);
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
            out = (char*)cJSON_malloc(len);
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
            tmplen = strlen(names[i]);
            memcpy(ptr, names[i], tmplen);
            ptr += tmplen;
            *ptr++ = ':';
            if (fmt)
            {
                *ptr++ = '\t';
            }
            strcpy(ptr, entries[i]);
            ptr += strlen(entries[i]);
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
int    cJSON_GetArraySize(const cJSON *array)
{
    cJSON *c = array->child;
    int i = 0;
    while(c)
    {
        i++;
        c = c->next;
    }
    return i;
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
    while (c && cJSON_strcasecmp(c->string, string))
    {
        c = c->next;
    }
    return c;
}

cjbool cJSON_HasObjectItem(const cJSON *object,const char *string)
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
void   cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    cJSON *c = array->child;
    if (!item)
    {
        return;
    }
    if (!c)
    {
        /* list is empty, start new one */
        array->child = item;
    }
    else
    {
        /* append to the end */
        while (c->next)
        {
            c = c->next;
        }
        suffix_object(c, item);
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
    item->string = cJSON_strdup(string);

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

cJSON *cJSON_DetachItemFromArray(cJSON *array, int which)
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

void cJSON_DeleteItemFromArray(cJSON *array, int which)
{
    cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}

cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string)
{
    int i = 0;
    cJSON *c = object->child;
    while (c && cJSON_strcasecmp(c->string,string))
    {
        i++;
        c = c->next;
    }
    if (c)
    {
        return cJSON_DetachItemFromArray(object, i);
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

void cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem)
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

void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem)
{
    int i = 0;
    cJSON *c = object->child;
    while(c && cJSON_strcasecmp(c->string, string))
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

        newitem->string = cJSON_strdup(string);
        cJSON_ReplaceItemInArray(object, i, newitem);
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
        item->valueint = (int)num;
    }

    return item;
}

cJSON *cJSON_CreateString(const char *string)
{
    cJSON *item = cJSON_New_Item();
    if(item)
    {
        item->type = cJSON_String;
        item->valuestring = cJSON_strdup(string);
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
        item->valuestring = cJSON_strdup(raw);
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
    int i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = cJSON_CreateArray();
    for(i = 0; a && (i < count); i++)
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
    int i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = cJSON_CreateArray();
    for(i = 0; a && (i < count); i++)
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
    int i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = cJSON_CreateArray();
    for(i = 0;a && (i < count); i++)
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
    int i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = cJSON_CreateArray();
    for (i = 0; a && (i < count); i++)
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
    cJSON *cptr = NULL;
    cJSON *nptr = NULL;
    cJSON *newchild = NULL;

    /* Bail on bad ptr */
    if (!item)
    {
        return NULL;
    }
    /* Create new item */
    newitem = cJSON_New_Item();
    if (!newitem)
    {
        return NULL;
    }
    /* Copy over all vars */
    newitem->type = item->type & (~cJSON_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring)
    {
        newitem->valuestring = cJSON_strdup(item->valuestring);
        if (!newitem->valuestring)
        {
            cJSON_Delete(newitem);
            return NULL;
        }
    }
    if (item->string)
    {
        newitem->string = (item->type&cJSON_StringIsConst) ? item->string : cJSON_strdup(item->string);
        if (!newitem->string)
        {
            cJSON_Delete(newitem);
            return NULL;
        }
    }
    /* If non-recursive, then we're done! */
    if (!recurse)
    {
        return newitem;
    }
    /* Walk the ->next chain for the child. */
    cptr = item->child;
    while (cptr)
    {
        newchild = cJSON_Duplicate(cptr, 1); /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild)
        {
            cJSON_Delete(newitem);
            return NULL;
        }
        if (nptr)
        {
            /* If newitem->child already set, then crosswire ->prev and ->next and move on */
            nptr->next = newchild;
            newchild->prev = nptr;
            nptr = newchild;
        }
        else
        {
            /* Set newitem->child and move to it */
            newitem->child = newchild; nptr = newchild;
        }
        cptr = cptr->next;
    }

    return newitem;
}

void cJSON_Minify(char *json)
{
    char *into = json;
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
            *into++ = *json++;
            while (*json && (*json != '\"'))
            {
                if (*json == '\\')
                {
                    *into++=*json++;
                }
                *into++ = *json++;
            }
            *into++ = *json++;
        }
        else
        {
            /* All other characters. */
            *into++ = *json++;
        }
    }

    /* and null-terminate. */
    *into = '\0';
}
