/*
 * Copyright (c) 2023, smartmx - smartmx@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef _CJSON_CONFIG_H_
#define _CJSON_CONFIG_H_

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>

#define cJSON_INLINE        __attribute__((always_inline)) static inline

/* malloc/free objects using the malloc/free functions */
cJSON_INLINE void *cJSON_malloc(size_t size)
{
    return malloc(size);
}

cJSON_INLINE void cJSON_free(void *object)
{
    free(object);
}

/*
 * config default sprintf function for cJSON
 */
#define cJSON_SPRINTF       sprintf

/*
 * config default strlen function for cJSON
 */
#define cJSON_STRLEN        strlen

/*
 * config default strcpy function for cJSON
 */
#define cJSON_STRCPY        strcpy

/*
 * config default strncmp function for cJSON
 */
#define cJSON_STRNCMP       strncmp

/*
 * config default strtod function for cJSON
 */
#define cJSON_STRTOD        strtod

/*
 * config default sscanf function for cJSON
 */
#define cJSON_SSCANF        sscanf

/*
 * config default memcpy function for cJSON
 */
#define cJSON_MEMCPY        memcpy

/*
 * config default memset function for cJSON
 */
#define cJSON_MEMSET        memset

/*
 * config default fabs function for cJSON
 */
#define cJSON_FABS          fabs

#endif /* _CJSON_CONFIG_H_ */
