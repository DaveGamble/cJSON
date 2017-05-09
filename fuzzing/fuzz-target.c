/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

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

#include <string.h>
#include <stdio.h>

#include "fuzz-target.h"
#include "../cJSON.h"

static void minify(const unsigned char *data, size_t size)
{
    unsigned char *copied_data = (unsigned char*)malloc(size);
    if (copied_data == NULL)
    {
        return;
    }

    memcpy(copied_data, data, size);

    cJSON_Minify((char*)copied_data);

    free(copied_data);

    return;
}

static void printing(cJSON *json, unsigned char format_setting, unsigned char buffered_setting)
{
    unsigned char *printed = NULL;

    if (buffered_setting == '1')
    {
        printed = (unsigned char*)cJSON_PrintBuffered(json, 1, (format_setting == '1'));
    }
    else
    {
        if (format_setting == '1')
        {
            printed = (unsigned char*)cJSON_Print(json);
        }
        else
        {
            printed = (unsigned char*)cJSON_PrintUnformatted(json);
        }
    }

    if (printed != NULL)
    {
        free(printed);
    }
}

extern int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    unsigned char minify_setting = '\0'; /* minify instead of parsing */
    unsigned char require_zero_setting = '\0'; /* zero termination required */
    unsigned char format_setting = '\0'; /* formatted printing */
    unsigned char buffered_setting = '\0'; /* buffered printing */
    const size_t data_offset = 4;

    cJSON *json = NULL;

    /* don't work with NULL or without mode selector */
    if ((data == NULL) || (size < data_offset))
    {
        return 0;
    }

    /* get configuration from the beginning of the test case */
    minify_setting = data[0];
    require_zero_setting = data[1];
    format_setting = data[2];
    buffered_setting = data[3];

    /* check if settings are valid */
    if ((minify_setting != '0') && (minify_setting != '1'))
    {
        return 0;
    }
    if ((require_zero_setting != '0') && (require_zero_setting != '1'))
    {
        return 0;
    }
    if ((format_setting != '0') && (format_setting != '1'))
    {
        return 0;
    }
    if ((buffered_setting != '0') && (buffered_setting != '1'))
    {
        return 0;
    }

    if (minify_setting == '1')
    {
        minify(data + data_offset, size);
        return 0;
    }

    json = cJSON_ParseWithOpts((const char*)data + data_offset, NULL, (require_zero_setting == '1'));
    if (json == NULL)
    {
        return 0;
    }

    printing(json, format_setting, buffered_setting);

    free(json);

    return 0;
}
