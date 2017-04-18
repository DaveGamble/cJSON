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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON_Utils.h"

int main(void)
{
    /* Some variables */
    int i = 0;

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

    return 0;
}
