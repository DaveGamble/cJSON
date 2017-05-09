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

#include "fuzz-target.h"

static char *read_file(const char *filename, size_t *size)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    if (size == NULL)
    {
        goto fail;
    }

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        goto fail;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        goto fail;
    }
    length = ftell(file);
    if (length < 0)
    {
        goto fail;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        goto fail;
    }

    /* allocate content buffer */
    content = (char*)malloc((size_t)length + sizeof(""));
    if (content == NULL)
    {
        goto fail;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length)
    {
        goto fail;
    }
    content[read_chars] = '\0';

    *size = read_chars + sizeof("");

    goto cleanup;

fail:
    if (size != NULL)
    {
        *size = 0;
    }
    if (content != NULL)
    {
        free(content);
        content = NULL;
    }

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
    char *json = NULL;
    int status = EXIT_FAILURE;
    char *printed_json = NULL;

    if (argc != 2)
    {
        printf("Usage:\n");
        printf("%s input_file\n", argv[0]);
        printf("\t input_file: file containing the test data\n");
        goto cleanup;
    }

    filename = argv[1];

#if defined(__AFL_HAVE_MANUAL_CONTROL) && __AFL_HAVE_MANUAL_CONTROL
    while (__AFL_LOOP(1000))
    {
#else
    {
#endif
        size_t size = 0;
        status = EXIT_SUCCESS;

        json = read_file(filename, &size);
        if ((json == NULL) || (json[0] == '\0') || (json[1] == '\0'))
        {
            status = EXIT_FAILURE;
            goto cleanup;
        }

        LLVMFuzzerTestOneInput(json, size);

    cleanup:
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
    }

    return status;
}
