CCFLAGS = -std=c99 -Wall

default: crush/*.c
	mkdir -p bin
	cc ${CCFLAGS} -o bin/crush crush/main.c
