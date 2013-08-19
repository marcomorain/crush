CCFLAGS = -std=c99 -Wall -O3

default: crush/*.c
	mkdir -p bin
	cc ${CCFLAGS} -o bin/crush crush/main.c crush/crush.c
