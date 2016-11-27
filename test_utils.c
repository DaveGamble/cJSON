#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON_Utils.h"

int main(void)
{
    /* Some variables */
    char *temp = NULL;
    char *patchtext = NULL;
    char *patchedtext = NULL;

    int i = 0;
    /* JSON Pointer tests: */
    cJSON *root = NULL;
    const char *json=
        "{"
        "\"foo\": [\"bar\", \"baz\"],"
        "\"\": 0,"
        "\"a/b\": 1,"
        "\"c%d\": 2,"
        "\"e^f\": 3,"
        "\"g|h\": 4,"
        "\"i\\\\j\": 5,"
        "\"k\\\"l\": 6,"
        "\" \": 7,"
        "\"m~n\": 8"
        "}";

    const char *tests[12] = {"","/foo","/foo/0","/","/a~1b","/c%d","/e^f","/g|h","/i\\j","/k\"l","/ ","/m~0n"};

    /* JSON Apply Patch tests: */
    const char *patches[15][3] =
    {
        {"{ \"foo\": \"bar\"}", "[{ \"op\": \"add\", \"path\": \"/baz\", \"value\": \"qux\" }]","{\"baz\": \"qux\",\"foo\": \"bar\"}"},
        {"{ \"foo\": [ \"bar\", \"baz\" ] }", "[{ \"op\": \"add\", \"path\": \"/foo/1\", \"value\": \"qux\" }]","{\"foo\": [ \"bar\", \"qux\", \"baz\" ] }"},
        {"{\"baz\": \"qux\",\"foo\": \"bar\"}"," [{ \"op\": \"remove\", \"path\": \"/baz\" }]","{\"foo\": \"bar\" }"},
        {"{ \"foo\": [ \"bar\", \"qux\", \"baz\" ] }","[{ \"op\": \"remove\", \"path\": \"/foo/1\" }]","{\"foo\": [ \"bar\", \"baz\" ] }"},
        {"{ \"baz\": \"qux\",\"foo\": \"bar\"}","[{ \"op\": \"replace\", \"path\": \"/baz\", \"value\": \"boo\" }]","{\"baz\": \"boo\",\"foo\": \"bar\"}"},
        {"{\"foo\": {\"bar\": \"baz\",\"waldo\": \"fred\"},\"qux\": {\"corge\": \"grault\"}}","[{ \"op\": \"move\", \"from\": \"/foo/waldo\", \"path\": \"/qux/thud\" }]","{\"foo\": {\"bar\": \"baz\"},\"qux\": {\"corge\": \"grault\",\"thud\": \"fred\"}}"},
        {"{ \"foo\": [ \"all\", \"grass\", \"cows\", \"eat\" ] }","[ { \"op\": \"move\", \"from\": \"/foo/1\", \"path\": \"/foo/3\" }]","{ \"foo\": [ \"all\", \"cows\", \"eat\", \"grass\" ] }"},
        {"{\"baz\": \"qux\",\"foo\": [ \"a\", 2, \"c\" ]}","[{ \"op\": \"test\", \"path\": \"/baz\", \"value\": \"qux\" },{ \"op\": \"test\", \"path\": \"/foo/1\", \"value\": 2 }]",""},
        {"{ \"baz\": \"qux\" }","[ { \"op\": \"test\", \"path\": \"/baz\", \"value\": \"bar\" }]",""},
        {"{ \"foo\": \"bar\" }","[{ \"op\": \"add\", \"path\": \"/child\", \"value\": { \"grandchild\": { } } }]","{\"foo\": \"bar\",\"child\": {\"grandchild\": {}}}"},
        {"{ \"foo\": \"bar\" }","[{ \"op\": \"add\", \"path\": \"/baz\", \"value\": \"qux\", \"xyz\": 123 }]","{\"foo\": \"bar\",\"baz\": \"qux\"}"},
        {"{ \"foo\": \"bar\" }","[{ \"op\": \"add\", \"path\": \"/baz/bat\", \"value\": \"qux\" }]",""},
        {"{\"/\": 9,\"~1\": 10}","[{\"op\": \"test\", \"path\": \"/~01\", \"value\": 10}]",""},
        {"{\"/\": 9,\"~1\": 10}","[{\"op\": \"test\", \"path\": \"/~01\", \"value\": \"10\"}]",""},
        {"{ \"foo\": [\"bar\"] }","[ { \"op\": \"add\", \"path\": \"/foo/-\", \"value\": [\"abc\", \"def\"] }]","{\"foo\": [\"bar\", [\"abc\", \"def\"]] }"}
    };

    /* JSON Apply Merge tests: */
    const char *merges[15][3] =
    {
        {"{\"a\":\"b\"}", "{\"a\":\"c\"}", "{\"a\":\"c\"}"},
        {"{\"a\":\"b\"}", "{\"b\":\"c\"}", "{\"a\":\"b\",\"b\":\"c\"}"},
        {"{\"a\":\"b\"}", "{\"a\":null}", "{}"},
        {"{\"a\":\"b\",\"b\":\"c\"}", "{\"a\":null}", "{\"b\":\"c\"}"},
        {"{\"a\":[\"b\"]}", "{\"a\":\"c\"}", "{\"a\":\"c\"}"},
        {"{\"a\":\"c\"}", "{\"a\":[\"b\"]}", "{\"a\":[\"b\"]}"},
        {"{\"a\":{\"b\":\"c\"}}", "{\"a\":{\"b\":\"d\",\"c\":null}}", "{\"a\":{\"b\":\"d\"}}"},
        {"{\"a\":[{\"b\":\"c\"}]}", "{\"a\":[1]}", "{\"a\":[1]}"},
        {"[\"a\",\"b\"]", "[\"c\",\"d\"]", "[\"c\",\"d\"]"},
        {"{\"a\":\"b\"}", "[\"c\"]", "[\"c\"]"},
        {"{\"a\":\"foo\"}", "null", "null"},
        {"{\"a\":\"foo\"}", "\"bar\"", "\"bar\""},
        {"{\"e\":null}", "{\"a\":1}", "{\"e\":null,\"a\":1}"},
        {"[1,2]", "{\"a\":\"b\",\"c\":null}", "{\"a\":\"b\"}"},
        {"{}","{\"a\":{\"bb\":{\"ccc\":null}}}", "{\"a\":{\"bb\":{}}}"}
    };


    /* Misc tests */
    int numbers[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const char *random = "QWERTYUIOPASDFGHJKLZXCVBNM";
    char buf[2] = {0,0};
    char *before = NULL;
    char *after = NULL;
    cJSON *object = NULL;
    cJSON *nums = NULL;
    cJSON *num6 = NULL;
    cJSON *sortme = NULL;


    printf("JSON Pointer Tests\n");
    root = cJSON_Parse(json);
    for (i = 0; i < 12; i++)
    {
        char *output = cJSON_Print(cJSONUtils_GetPointer(root, tests[i]));
        printf("Test %d:\n%s\n\n", i + 1, output);
        free(output);
    }
    cJSON_Delete(root);


    printf("JSON Apply Patch Tests\n");
    for (i = 0; i < 15; i++)
    {
        cJSON *object_to_be_patched = cJSON_Parse(patches[i][0]);
        cJSON *patch = cJSON_Parse(patches[i][1]);
        int err = cJSONUtils_ApplyPatches(object_to_be_patched, patch);
        char *output = cJSON_Print(object_to_be_patched);
        printf("Test %d (err %d):\n%s\n\n", i + 1, err, output);

        free(output);
        cJSON_Delete(object_to_be_patched);
        cJSON_Delete(patch);
    }

    /* JSON Generate Patch tests: */
    printf("JSON Generate Patch Tests\n");
    for (i = 0; i < 15; i++)
    {
        cJSON *from;
        cJSON *to;
        cJSON *patch;
        char *out;
        if (!strlen(patches[i][2]))
        {
            continue;
        }
        from = cJSON_Parse(patches[i][0]);
        to = cJSON_Parse(patches[i][2]);
        patch = cJSONUtils_GeneratePatches(from, to);
        out = cJSON_Print(patch);
        printf("Test %d: (patch: %s):\n%s\n\n", i + 1, patches[i][1], out);

        free(out);
        cJSON_Delete(from);
        cJSON_Delete(to);
        cJSON_Delete(patch);
    }

    /* Misc tests: */
    printf("JSON Pointer construct\n");
    object = cJSON_CreateObject();
    nums = cJSON_CreateIntArray(numbers, 10);
    num6 = cJSON_GetArrayItem(nums, 6);
    cJSON_AddItemToObject(object, "numbers", nums);
    temp = cJSONUtils_FindPointerFromObjectTo(object, num6);
    printf("Pointer: [%s]\n", temp);
    free(temp);
    temp = cJSONUtils_FindPointerFromObjectTo(object, nums);
    printf("Pointer: [%s]\n", temp);
    free(temp);
    temp = cJSONUtils_FindPointerFromObjectTo(object, object);
    printf("Pointer: [%s]\n", temp);
    free(temp);
    cJSON_Delete(object);

    /* JSON Sort test: */
    sortme = cJSON_CreateObject();
    for (i = 0; i < 26; i++)
    {
        buf[0] = random[i];
        cJSON_AddItemToObject(sortme, buf, cJSON_CreateNumber(1));
    }
    before = cJSON_PrintUnformatted(sortme);
    cJSONUtils_SortObject(sortme);
    after = cJSON_PrintUnformatted(sortme);
    printf("Before: [%s]\nAfter: [%s]\n\n", before, after);

    free(before);
    free(after);
    cJSON_Delete(sortme);

    /* Merge tests: */
    printf("JSON Merge Patch tests\n");
    for (i = 0; i < 15; i++)
    {
        cJSON *object_to_be_merged = cJSON_Parse(merges[i][0]);
        cJSON *patch = cJSON_Parse(merges[i][1]);
        char *before_merge = cJSON_PrintUnformatted(object_to_be_merged);
        patchtext = cJSON_PrintUnformatted(patch);
        printf("Before: [%s] -> [%s] = ", before_merge, patchtext);
        object_to_be_merged = cJSONUtils_MergePatch(object_to_be_merged, patch);
        after = cJSON_PrintUnformatted(object_to_be_merged);
        printf("[%s] vs [%s] (%s)\n", after, merges[i][2], strcmp(after, merges[i][2]) ? "FAIL" : "OK");

        free(before_merge);
        free(patchtext);
        free(after);
        cJSON_Delete(object_to_be_merged);
        cJSON_Delete(patch);
    }

    /* Generate Merge tests: */
    for (i = 0; i < 15; i++)
    {
        cJSON *from = cJSON_Parse(merges[i][0]);
        cJSON *to = cJSON_Parse(merges[i][2]);
        cJSON *patch = cJSONUtils_GenerateMergePatch(from,to);
        from = cJSONUtils_MergePatch(from,patch);
        patchtext = cJSON_PrintUnformatted(patch);
        patchedtext = cJSON_PrintUnformatted(from);
        printf("Patch [%s] vs [%s] = [%s] vs [%s] (%s)\n", patchtext, merges[i][1], patchedtext, merges[i][2], strcmp(patchedtext, merges[i][2]) ? "FAIL" : "OK");

        cJSON_Delete(from);
        cJSON_Delete(to);
        cJSON_Delete(patch);
        free(patchtext);
        free(patchedtext);
    }

    return 0;
}
