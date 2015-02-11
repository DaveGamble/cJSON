#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

	// JSON Apply Patch tests:
	const char *patches[15][3]={
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
	{"{ \"foo\": [\"bar\"] }","[ { \"op\": \"add\", \"path\": \"/foo/-\", \"value\": [\"abc\", \"def\"] }]","{\"foo\": [\"bar\", [\"abc\", \"def\"]] }"}};

	printf("JSON Apply Patch Tests\n");
	for (int i=0;i<15;i++)
	{
		cJSON *object=cJSON_Parse(patches[i][0]);
		cJSON *patch=cJSON_Parse(patches[i][1]);
		int err=cJSONUtils_ApplyPatches(object,patch);
		char *output=cJSON_Print(object);
		printf("Test %d (err %d):\n%s\n\n",i+1,err,output);
		free(output);cJSON_Delete(object);cJSON_Delete(patch);
	}

	// JSON Generate Patch tests:
	printf("JSON Generate Patch Tests\n");
	for (int i=0;i<15;i++)
	{
		if (!strlen(patches[i][2])) continue;
		cJSON *from=cJSON_Parse(patches[i][0]);
		cJSON *to=cJSON_Parse(patches[i][2]);
		cJSON *patch=cJSONUtils_GeneratePatches(from,to);
		char *out=cJSON_Print(patch);
		printf("Test %d: (patch: %s):\n%s\n\n",i+1,patches[i][1],out);
		free(out);cJSON_Delete(from);cJSON_Delete(to);cJSON_Delete(patch);
	}

	// Misc tests:
	printf("JSON Pointer construct\n");
	int numbers[10]={0,1,2,3,4,5,6,7,8,9};
	cJSON *object=cJSON_CreateObject();
	cJSON *nums=cJSON_CreateIntArray(numbers,10);
	cJSON *num6=cJSON_GetArrayItem(nums,6);
	cJSON_AddItemToObject(object,"numbers",nums);
	printf("Pointer: [%s]\n",cJSONUtils_FindPointerFromObjectTo(object,num6));
	printf("Pointer: [%s]\n",cJSONUtils_FindPointerFromObjectTo(object,nums));
	printf("Pointer: [%s]\n",cJSONUtils_FindPointerFromObjectTo(object,object));
	
}
