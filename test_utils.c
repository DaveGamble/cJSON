#include <stdlib.h>
#include <stdio.h>
#include "cJSON_Utils.h"

int main()
{
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
		printf("Test %d:\n%s\n\n",i+1,cJSON_Print(cJSONUtils_GetPointer(root,tests[i])));
	}


}
