/*
  Copyright (c) 2009-2019 Dave Gamble and cJSON contributors

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

#include "unity/examples/unity_config.h"
#include "unity/src/unity.h"
#include "common.h"


static void cjson_minify_should_not_overflow_buffer(void)
{
    char unclosed_multiline_comment[] = "/* bla";
    char pending_escape[] = "\"\\";

    cJSON_Minify(unclosed_multiline_comment);
    TEST_ASSERT_EQUAL_STRING("", unclosed_multiline_comment);

    cJSON_Minify(pending_escape);
    TEST_ASSERT_EQUAL_STRING("\"\\", pending_escape);
}

static void cjson_minify_should_remove_single_line_comments(void)
{
    const char to_minify[] = "{// this is {} \"some kind\" of [] comment /*, don't you see\n}";

    char* minified = (char*) malloc(sizeof(to_minify));
    TEST_ASSERT_NOT_NULL(minified);
    strcpy(minified, to_minify);

    cJSON_Minify(minified);
    TEST_ASSERT_EQUAL_STRING("{}", minified);

    free(minified);
}

static void cjson_minify_should_remove_spaces(void)
{
    const char to_minify[] = "{ \"key\":\ttrue\r\n    }";

    char* minified = (char*) malloc(sizeof(to_minify));
    TEST_ASSERT_NOT_NULL(minified);
    strcpy(minified, to_minify);

    cJSON_Minify(minified);
    TEST_ASSERT_EQUAL_STRING("{\"key\":true}", minified);

    free(minified);
}

static void cjson_minify_should_remove_multiline_comments(void)
{
    const char to_minify[] = "{/* this is\n a /* multi\n //line \n {comment \"\\\" */}";

    char* minified = (char*) malloc(sizeof(to_minify));
    TEST_ASSERT_NOT_NULL(minified);
    strcpy(minified, to_minify);

    cJSON_Minify(minified);
    TEST_ASSERT_EQUAL_STRING("{}", minified);

    free(minified);
}

static void cjson_minify_should_not_modify_strings(void)
{
    const char to_minify[] = "\"this is a string \\\" \\t bla\"";

    char* minified = (char*) malloc(sizeof(to_minify));
    TEST_ASSERT_NOT_NULL(minified);
    strcpy(minified, to_minify);

    cJSON_Minify(minified);
    TEST_ASSERT_EQUAL_STRING(to_minify, minified);

    free(minified);
}

static void cjson_minify_should_minify_json(void) {
    const char to_minify[] =
            "{\n"
            "    \"glossary\": { // comment\n"
            "        \"title\": \"example glossary\",\n"
            "  /* multi\n"
            " line */\n"
            "		\"GlossDiv\": {\n"
            "            \"title\": \"S\",\n"
            "			\"GlossList\": {\n"
            "                \"GlossEntry\": {\n"
            "                    \"ID\": \"SGML\",\n"
            "					\"SortAs\": \"SGML\",\n"
            "					\"Acronym\": \"SGML\",\n"
            "					\"Abbrev\": \"ISO 8879:1986\",\n"
            "					\"GlossDef\": {\n"
            "						\"GlossSeeAlso\": [\"GML\", \"XML\"]\n"
            "                    },\n"
            "					\"GlossSee\": \"markup\"\n"
            "                }\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "}";
    const char* minified =
            "{"
            "\"glossary\":{"
            "\"title\":\"example glossary\","
            "\"GlossDiv\":{"
            "\"title\":\"S\","
            "\"GlossList\":{"
            "\"GlossEntry\":{"
            "\"ID\":\"SGML\","
            "\"SortAs\":\"SGML\","
            "\"Acronym\":\"SGML\","
            "\"Abbrev\":\"ISO 8879:1986\","
            "\"GlossDef\":{"
            "\"GlossSeeAlso\":[\"GML\",\"XML\"]"
            "},"
            "\"GlossSee\":\"markup\""
            "}"
            "}"
            "}"
            "}"
            "}";

    char *buffer = (char*) malloc(sizeof(to_minify));
    strcpy(buffer, to_minify);

    cJSON_Minify(buffer);
    TEST_ASSERT_EQUAL_STRING(minified, buffer);

    free(buffer);
}

static void cjson_minify_should_not_loop_infinitely(void) {
    char string[] = { '8', ' ', '/', ' ', '5', '\n', '\0' };
    /* this should not be an infinite loop */
    cJSON_Minify(string);
}

int CJSON_CDECL main(void)
{
    UNITY_BEGIN();

    RUN_TEST(cjson_minify_should_not_overflow_buffer);
    RUN_TEST(cjson_minify_should_minify_json);
    RUN_TEST(cjson_minify_should_remove_single_line_comments);
    RUN_TEST(cjson_minify_should_remove_multiline_comments);
    RUN_TEST(cjson_minify_should_remove_spaces);
    RUN_TEST(cjson_minify_should_not_modify_strings);
    RUN_TEST(cjson_minify_should_not_loop_infinitely);

    return UNITY_END();
}
