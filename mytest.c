#include "cJSON.h"
#include <stdio.h>
#include <string.h>

char JSON_STR[] = "{ \"val\": \"\\u0000\\u0001\", \"vval\":true, \"test_plain\":\"\\r\\n\", \"plain_text\": \"i'm a text\", \"test_int64\": 18446744073709551610}";
int main(void) {
		printf("%s\n", JSON_STR);
		cJSON *json = cJSON_Parse(JSON_STR);
		if(json == NULL) {
				printf("could not parse\n");
				return 1;
		}
		cJSON *json1 = cJSON_GetObjectItem(json, "val");
		if(json1 == NULL) {
				printf("could not get val\n");
		}
		printf("val is : *%s*\n", cJSON_GetStringValue(json1));
		cJSON *json2 = cJSON_GetObjectItem(json, "test_plain");
		if(json2 == NULL) {
				printf("could not get val\n");
		}
		printf("val is : *%s*\n", cJSON_GetStringValue(json2));
		cJSON *json3 = cJSON_GetObjectItem(json, "plain_text");
		if(json3 == NULL) {
				printf("could not get val\n");
		}
		cJSON *json4 = cJSON_GetObjectItem(json, "test_int64");
		printf("got int 64 val: %016lx\n", json4->valueint64);
		double asDouble = cJSON_GetNumberValue(json4);
		int asInt = json4->valueint;
		printf("got val: %e %lx %u\n",asDouble, *((uint64_t*)&asDouble), asInt); 
		printf("val is : *%s*\nlen is : %lu as opposed to: %lu\n", cJSON_GetStringValue(json3), json3->valuestring_len, strlen(json3->valuestring));
		cJSON_Delete(json);
		return 0;
}
