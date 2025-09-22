#!/bin/sh

test () {
    OUTPUT="$(./bin/main $1)"
    if [ "${OUTPUT}" != "$2" ]; then
        echo "Error $1:"
        echo "  Expected $2, got ${OUTPUT}"
    else
        echo "Passed $1"
    fi
}

test "test/hi.b" "Hi"
test "test/helloworld.b" "Hello, World!"
