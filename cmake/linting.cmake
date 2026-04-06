# Lint and static/dynamic analysis tools
file(GLOB_RECURSE LINT_SOURCE_FILES "src/*.[ch]" "test/*.[ch]")

# cpplint
find_program(CPPLINT NAMES cpplint cpplint.py PATHS /usr/bin /usr/local/bin /opt/homebrew/bin)
if(NOT CPPLINT)
    message(WARNING "cpplint not found, cpplint step will be skipped in lint target.")
endif()

# clang-analyze
find_program(CLANG_ANALYZE NAMES clang PATHS /usr/bin /usr/local/bin /opt/homebrew/bin)
if(NOT CLANG_ANALYZE)
    message(WARNING "clang not found, static analysis will be skipped in lint target.")
endif()

if(CPPLINT)
    if(CLANG_ANALYZE AND LLVM_AVAILABLE)
        file(GLOB_RECURSE LINT_SOURCE_FILES "src/*.[ch]" "test/*.[ch]")
        if(LINT_SOURCE_FILES)
            add_custom_target(lint
                COMMAND ${CMAKE_COMMAND} -E echo "--- Running cpplint ---"
                COMMAND ${CPPLINT} --linelength=80 --quiet ${LINT_SOURCE_FILES}
                COMMAND ${CMAKE_COMMAND} -E echo "--- Running clang static analyzer ---"
                COMMAND ${CLANG_ANALYZE} --analyze -Xanalyzer -analyzer-output=text -I${LLVM_INCLUDE_DIRS} src/*.c
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Linting"
            )
            # Only depend on lint-scripts if not in Docker build mode
            if(NOT DOCKER_BUILD AND TARGET lint-scripts)
                add_dependencies(lint lint-scripts)
            endif()
        else()
            message(STATUS "No source files found for cpplint")
            add_custom_target(lint
                COMMAND echo "No source files to lint"
                COMMENT "Lint target (cpplint) disabled - no files found"
            )
        endif()
    else()
        if(CLANG_ANALYZE AND NOT LLVM_AVAILABLE)
            message(WARNING "LLVM not available, static analysis will be skipped in lint target.")
        elseif(NOT CLANG_ANALYZE)
            message(WARNING "clang not found, static analysis will be skipped in lint target.")
        endif()
        message(STATUS "Lint target will run cpplint only when LLVM is unavailable.")
        file(GLOB_RECURSE LINT_SOURCE_FILES "src/*.[ch]" "test/*.[ch]")
        if(LINT_SOURCE_FILES)
            add_custom_target(lint
                COMMAND ${CMAKE_COMMAND} -E echo "--- Running cpplint ---"
                COMMAND ${CPPLINT} --linelength=80 --quiet ${LINT_SOURCE_FILES}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Linting"
            )
            if(NOT DOCKER_BUILD AND TARGET lint-scripts)
                add_dependencies(lint lint-scripts)
            endif()
        else()
            message(STATUS "No source files found for cpplint")
            add_custom_target(lint
                COMMAND echo "No source files to lint"
                COMMENT "Lint target (cpplint) disabled - no files found"
            )
        endif()
    endif()
else()
    message(WARNING "cpplint not found, lint target will not be available")
    add_custom_target(lint
        COMMAND echo "Lint not available - cpplint not found"
        COMMENT "Lint target disabled"
    )
endif()