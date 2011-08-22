all: cJSON.c test.c
	gcc cJSON.c test.c -o test -lm