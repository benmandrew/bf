test () {
  OUTPUT="$(./bin/main $1)"
  if [ "${OUTPUT}" != "$2" ]; then
    echo "Error $1:"
    echo "  Expected $2, got ${OUTPUT}"
  fi
}

test "test/helloworld.b" "Hello, World!"

