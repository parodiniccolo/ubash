CC = gcc
CFLAGS = -Werror -std=c11 -pedantic

.PHONY: default all clean


all: ubash

ubash: ubash.o
ubash.o: ubash.c 

clean:
	rm -f ubash *.o
