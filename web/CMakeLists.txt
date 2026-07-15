set(SITE_URL "http://localhost:8000/compile?code="
    CACHE STRING "URL for the BF compile endpoint used in the site build")

# theme=light keeps the graph light for the light site embed (style.site.css);
# the /cfg endpoint defaults to dark when no theme is given.
set(SITE_CFG_URL "http://localhost:8000/cfg?theme=light&code="
    CACHE STRING "URL for the BF control-flow-graph endpoint used in the site build")

set(SITE_DIST_DIR ${CMAKE_BINARY_DIR}/site)

add_custom_target(site
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SITE_DIST_DIR}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/app.js
        -DOUTPUT=${SITE_DIST_DIR}/bf.js
        -DPLACEHOLDER=__BF_COMPILE_URL__
        -DVALUE=${SITE_URL}
        -P ${CMAKE_SOURCE_DIR}/cmake/substitute.cmake
    # Second pass over the just-written bf.js for the CFG endpoint URL.
    COMMAND ${CMAKE_COMMAND}
        -DINPUT=${SITE_DIST_DIR}/bf.js
        -DOUTPUT=${SITE_DIST_DIR}/bf.js
        -DPLACEHOLDER=__BF_CFG_URL__
        -DVALUE=${SITE_CFG_URL}
        -P ${CMAKE_SOURCE_DIR}/cmake/substitute.cmake
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/style.site.css
        ${SITE_DIST_DIR}/bf.css
    COMMENT "Building site assets"
    VERBATIM
)
