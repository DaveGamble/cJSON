all: cJSON.o
	cc -Wall cJSON.c -c -o cJSON.o
	cc -Wall cJSON.o test.c -lm