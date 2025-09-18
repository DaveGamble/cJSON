#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size); /* required by C89 */

/* fuzz target entry point, works without libFuzzer */

int main(int argc, char **argv)
{
    FILE *f = NULL;
    char *buf = NULL;
    long siz_buf;

    if(argc < 2)
    {
        fprintf(stderr, "no input file\n");
        goto err;
    }

    f = fopen(argv[1], "rb");
    if(f == NULL)
    {
        fprintf(stderr, "error opening input file %s\n", argv[1]);
        goto err;
    }

    if(fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "fseek() to end failed\n");
        goto err;
    }

    siz_buf = ftell(f);
    rewind(f);

    if(siz_buf < 1) goto err;

    buf = (char*)malloc((size_t)siz_buf);
    if(buf == NULL)
    {
        fprintf(stderr, "malloc() failed\n");
        goto err;
    }

    if(fread(buf, (size_t)siz_buf, 1, f) != 1)
    {
        fprintf(stderr, "fread() failed\n");
        goto err;
    }

    (void)LLVMFuzzerTestOneInput((uint8_t*)buf, (size_t)siz_buf);

err:
    free(buf);
    if (f != NULL)
    {
        fclose(f);
    }

    return 0;
}
