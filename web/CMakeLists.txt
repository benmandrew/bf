# Stage the client-side static site into the build tree. The compiler now runs
# in the browser (web/worker.js loads bfc.wasm + Graphviz), so there is no
# endpoint URL to substitute the way this target used to: it just assembles the
# static assets. The wasm module itself is produced separately by
# scripts/build-wasm.sh into web/wasm/ and copied in if present.

set(SITE_DIST_DIR ${CMAKE_BINARY_DIR}/site)

set(SITE_STATIC_FILES
    index.html
    app.js
    worker.js
    highlight.js
    style.css
    prism.css
    prism.js
    favicon.ico
)

set(_site_copy_commands
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SITE_DIST_DIR}
)
foreach(f ${SITE_STATIC_FILES})
    list(APPEND _site_copy_commands
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${f} ${SITE_DIST_DIR}/${f})
endforeach()

add_custom_target(site
    ${_site_copy_commands}
    # Vendored Graphviz wasm module and its licence.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor ${SITE_DIST_DIR}/vendor
    # The bfc wasm module, if it has been built. Warn rather than fail so the
    # target is useful without the Emscripten toolchain on PATH.
    COMMAND ${CMAKE_COMMAND}
        -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/wasm
        -DDST=${SITE_DIST_DIR}/wasm
        -P ${CMAKE_CURRENT_SOURCE_DIR}/copy-wasm.cmake
    COMMENT "Staging client-side static site into ${SITE_DIST_DIR}"
    VERBATIM
)
