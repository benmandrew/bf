.PHONY: all clean

all: bin/main

bin/main: src/main.c build_dir
	gcc -o $@ $<

build_dir:
	mkdir -p bin

clean:
	rm -f bin/main

