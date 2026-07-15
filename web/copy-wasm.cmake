# Copy the built wasm module into the site bundle if it exists, warning
# instead of failing when it has not been built (scripts/build-wasm.sh).
if(EXISTS "${SRC}/bfc.wasm")
    file(MAKE_DIRECTORY "${DST}")
    file(COPY "${SRC}/" DESTINATION "${DST}")
    message(STATUS "Staged wasm module from ${SRC}")
else()
    message(WARNING
        "web/wasm/bfc.wasm not found; run scripts/build-wasm.sh first. "
        "The staged site will not compile in the browser without it.")
endif()
