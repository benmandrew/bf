find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        "src/*.c" "src/*.h"
        "test/*.c" "test/*.h"
    )

    add_custom_target(fmt
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting source files"
    )
    # Only depend on fmt-scripts if not in Docker build mode
    if(NOT DOCKER_BUILD AND TARGET fmt-scripts)
        add_dependencies(fmt fmt-scripts)
    endif()

    add_custom_target(fmt-ci
        COMMAND ${CLANG_FORMAT} --dry-run -Werror -i ${ALL_SOURCE_FILES}
        COMMENT "Checking source code formatting"
    )
    # fmt-ci is the target CI runs, so the script check has to hang off it
    # too; without this, scripts/ would only be checked by a manual fmt.
    if(NOT DOCKER_BUILD AND TARGET fmt-scripts-ci)
        add_dependencies(fmt-ci fmt-scripts-ci)
    endif()
else()
    message(WARNING "clang-format not found, formatting targets will not be available")
endif()