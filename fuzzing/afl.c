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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cJSON.h"

static char *read_file(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        goto cleanup;
    }
    length = ftell(file);
    if (length < 0)
    {
        goto cleanup;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    /* allocate content buffer */
    content = (char*)malloc((size_t)length + sizeof(""));
    if (content == NULL)
    {
        goto cleanup;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length)
    {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';


cleanup:
    if (file != NULL)
    {
        fclose(file);
    }

    return content;
}

int main(int argc, char** argv)
{
    const char *filename = NULL;
    cJSON *item = NULL;
    char *json = NULL;
    int status = EXIT_FAILURE;
    char *printed_json = NULL;

    if ((argc < 2) || (argc > 3))
    {
        printf("Usage:\n");
        printf("%s input_file [enable_printing]\n", argv[0]);
        printf("\t input_file: file containing the test data\n");
        printf("\t enable_printing: print after parsing, 'yes' or 'no', defaults to 'no'\n");
        goto cleanup;
    }

    filename = argv[1];

#if __AFL_HAVE_MANUAL_CONTROL
    while (__AFL_LOOP(1000))
    {
#endif
    status = EXIT_SUCCESS;

    json = read_file(filename);
    if ((json == NULL) || (json[0] == '\0') || (json[1] == '\0'))
    {
        status = EXIT_FAILURE;
        goto cleanup;
    }
    item = cJSON_Parse(json + 2);
    if (item == NULL)
    {
        goto cleanup;
    }

    if ((argc == 3) && (strncmp(argv[2], "yes", 3) == 0))
    {
        int do_format = 0;
        if (json[1] == 'f')
        {
            do_format = 1;
        }

        if (json[0] == 'b')
        {
            /* buffered printing */
            printed_json = cJSON_PrintBuffered(item, 1, do_format);
        }
        else
        {
            /* unbuffered printing */
            if (do_format)
            {
                printed_json = cJSON_Print(item);
            }
            else
            {
                printed_json = cJSON_PrintUnformatted(item);
            }
        }
        if (printed_json == NULL)
        {
            status = EXIT_FAILURE;
            goto cleanup;
        }
        printf("%s\n", printed_json);
    }

cleanup:
    if (item != NULL)
    {
        cJSON_Delete(item);
        item = NULL;
    }
    if (json != NULL)
    {
        free(json);
        json = NULL;
    }
    if (printed_json != NULL)
    {
        free(printed_json);
        printed_json = NULL;
    }
#if __AFL_HAVE_MANUAL_CONTROL
    }
#endif

    return status;
}
