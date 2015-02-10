#include <stdlib.h>
#include <stdio.h>
#include "cJSON_Utils.h"

int main()
{
	// JSON Pointer tests:
	const char *json="{"
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
   
	const char *tests[12]={"","/foo","/foo/0","/","/a~1b","/c%d","/e^f","/g|h","/i\\j","/k\"l","/ ","/m~0n"};
   
	printf("JSON Pointer Tests\n");
	cJSON *root=cJSON_Parse(json);
	for (int i=0;i<12;i++)
	{
		char *output=cJSON_Print(cJSONUtils_GetPointer(root,tests[i]));
		printf("Test %d:\n%s\n\n",i+1,output);
		free(output);
	}
	cJSON_Delete(root);

	// JSON Patch tests:
	const char *patches[15][2]={
	{"{ \"foo\": \"bar\"}", "[{ \"op\": \"add\", \"path\": \"/baz\", \"value\": \"qux\" }]"},
	{"{ \"foo\": [ \"bar\", \"baz\" ] }", "[{ \"op\": \"add\", \"path\": \"/foo/1\", \"value\": \"qux\" }]"},
	{"{\"baz\": \"qux\",\"foo\": \"bar\"}"," [{ \"op\": \"remove\", \"path\": \"/baz\" }]"},
	{"{ \"foo\": [ \"bar\", \"qux\", \"baz\" ] }","[{ \"op\": \"remove\", \"path\": \"/foo/1\" }]"},
	{"{ \"baz\": \"qux\",\"foo\": \"bar\"}","[{ \"op\": \"replace\", \"path\": \"/baz\", \"value\": \"boo\" }]"},
	{"{\"foo\": {\"bar\": \"baz\",\"waldo\": \"fred\"},\"qux\": {\"corge\": \"grault\"}}","[{ \"op\": \"move\", \"from\": \"/foo/waldo\", \"path\": \"/qux/thud\" }]"},
	{"{ \"foo\": [ \"all\", \"grass\", \"cows\", \"eat\" ] }","[ { \"op\": \"move\", \"from\": \"/foo/1\", \"path\": \"/foo/3\" }]"},
	{"{\"baz\": \"qux\",\"foo\": [ \"a\", 2, \"c\" ]}","[{ \"op\": \"test\", \"path\": \"/baz\", \"value\": \"qux\" },{ \"op\": \"test\", \"path\": \"/foo/1\", \"value\": 2 }]"},
	{"{ \"baz\": \"qux\" }","[ { \"op\": \"test\", \"path\": \"/baz\", \"value\": \"bar\" }]"},
	{"{ \"foo\": \"bar\" }","[{ \"op\": \"add\", \"path\": \"/child\", \"value\": { \"grandchild\": { } } }]"},
	{"{ \"foo\": \"bar\" }","[{ \"op\": \"add\", \"path\": \"/baz\", \"value\": \"qux\", \"xyz\": 123 }]"},
	{"{ \"foo\": \"bar\" }","[{ \"op\": \"add\", \"path\": \"/baz/bat\", \"value\": \"qux\" }]"},
	{"{\"/\": 9,\"~1\": 10}","[{\"op\": \"test\", \"path\": \"/~01\", \"value\": 10}]"},
	{"{\"/\": 9,\"~1\": 10}","[{\"op\": \"test\", \"path\": \"/~01\", \"value\": \"10\"}]"},
	{"{ \"foo\": [\"bar\"] }","[ { \"op\": \"add\", \"path\": \"/foo/-\", \"value\": [\"abc\", \"def\"] }]"}};

	printf("JSON Patch Tests\n");
	for (int i=0;i<15;i++)
	{
		cJSON *object=cJSON_Parse(patches[i][0]);
		cJSON *patch=cJSON_Parse(patches[i][1]);
		int err=cJSONUtils_ApplyPatches(object,patch);
		char *output=cJSON_Print(object);
		printf("Test %d (err %d):\n%s\n\n",i+1,err,output);
		free(output);	
	}

	
}
