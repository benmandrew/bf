.PHONY: all clean test

CFLAGS := -g -std=c17

SRC_FILES := src/main.c src/read.c src/interp.c

all: bin/main

bin/main: $(SRC_FILES)
	gcc $(CFLAGS) -o $@ $^

test: bin/main
	./test/test.sh

clean:
	rm -f bin/main
