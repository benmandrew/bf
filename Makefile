.PHONY: all clean

all: bin/main

bin/main: src/main.c src/read.c src/interp.c
	gcc -std=c17 -o $@ $^

clean:
	rm -f bin/main

