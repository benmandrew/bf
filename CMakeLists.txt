cmake_minimum_required(VERSION 3.20)
project(bf VERSION 1.0.0 LANGUAGES C CXX)

set(LLVM_POSSIBLE_PATHS
    # Homebrew paths (macOS)
    /opt/homebrew/opt/llvm
    /usr/local/opt/llvm
    # System paths
    /usr/lib/llvm-18
    /usr/lib/llvm-17
    /usr/lib/llvm-16
    /usr/lib/llvm-15
    /usr/lib/llvm-14
    /usr/lib/llvm-13
    /usr/lib/llvm-12
    /usr/lib/llvm-11
    /usr/lib/llvm-10
    # Other common paths
    /usr/local/llvm
    /opt/llvm
)

find_package(LLVM CONFIG QUIET)

if(NOT LLVM_FOUND)
    foreach(path ${LLVM_POSSIBLE_PATHS})
        if(EXISTS ${path})
            list(APPEND CMAKE_PREFIX_PATH ${path})
            find_package(LLVM CONFIG QUIET)
            if(LLVM_FOUND)
                message(STATUS "Found LLVM at: ${path}")
                break()
            endif()
        endif()
    endforeach()
endif()

if(NOT LLVM_FOUND)
    message(FATAL_ERROR "LLVM is required but not found. Install LLVM and try again.\n"
                        "Installation instructions:\n"
                        "macOS (Homebrew):\n"
                        "  brew install llvm\n"
                        "Ubuntu/Debian:\n"
                        "  sudo apt-get install llvm-dev\n")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)  # For gnu17

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Debug: Print LLVM configuration
message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_LIBRARY_DIRS: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")

# Use LLVM's library directory for linking
if(LLVM_LIBRARY_DIRS)
    link_directories(${LLVM_LIBRARY_DIRS})
endif()

llvm_map_components_to_libnames(llvm_libs support core irreader)

set(LLVM_AVAILABLE TRUE)

# Use an OBJECT library for shared sources to avoid flag leakage
set(LIB_SOURCES
    src/interp.c
    src/ir.c
    src/read.c
    src/llvm.c
)

add_library(bf_obj OBJECT ${LIB_SOURCES})
target_include_directories(bf_obj PUBLIC src)
target_compile_definitions(bf_obj PUBLIC LLVM_AVAILABLE)
target_compile_options(bf_obj PRIVATE -fPIE)

add_library(bf_lib STATIC $<TARGET_OBJECTS:bf_obj>)
target_include_directories(bf_lib PUBLIC src)
target_link_libraries(bf_lib ${llvm_libs})
target_compile_definitions(bf_lib PUBLIC LLVM_AVAILABLE)



# bfi executable
add_executable(bfi src/main_bfi.c)
target_link_libraries(bfi bf_lib)
target_compile_options(bfi PRIVATE -fPIE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -g -O1>
)
target_link_options(bfi PRIVATE -pie
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
set_target_properties(bfi bf_lib 
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# bfc executable
add_executable(bfc src/main_bfc.c)
target_link_libraries(bfc bf_lib)
target_compile_options(bfc PRIVATE -fPIE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -g -O1>
)
target_link_options(bfc PRIVATE -pie
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
set_target_properties(bfc bf_lib 
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Debug
    COMMENT "Building in debug mode"
)

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

    add_custom_target(fmt-ci
        COMMAND ${CLANG_FORMAT} --dry-run -Werror -i ${ALL_SOURCE_FILES}
        COMMENT "Checking source code formatting"
    )
else()
    message(WARNING "clang-format not found, formatting targets will not be available")
endif()

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
    if (CLANG_ANALYZE)
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
        else()
            message(STATUS "No source files found for cpplint")
            add_custom_target(lint
                COMMAND echo "No source files to lint"
                COMMENT "Lint target (cpplint) disabled - no files found"
            )
        endif()
    else()
        message(WARNING "clang not found, lint target will not be available")
        add_custom_target(lint
            COMMAND echo "Lint not available - clang not found"
            COMMENT "Lint target disabled"
        )
    endif()
else()
    message(WARNING "cpplint not found, lint target will not be available")
    add_custom_target(lint
        COMMAND echo "Lint not available - cpplint not found"
        COMMENT "Lint target disabled"
    )
endif()

# AFL fuzzing and sanitizer/valgrind checks
find_program(AFL_CC NAMES afl-clang-fast afl-gcc afl-clang PATHS /usr/bin /usr/local/bin /opt/homebrew/bin)
if(NOT AFL_CC)
    message(WARNING "AFL compiler not found, fuzz target will be skipped.")
endif()

# AFL fuzzing and sanitizer/valgrind checks
if(AFL_CC)
    add_library(bf_obj_fuzz OBJECT ${LIB_SOURCES})
    target_include_directories(bf_obj_fuzz PUBLIC src)
    target_compile_definitions(bf_obj_fuzz PUBLIC LLVM_AVAILABLE)
    target_compile_options(bf_obj_fuzz PRIVATE -fsanitize=address,undefined -g -O1 -fPIE)

    add_library(bf_lib_fuzz STATIC $<TARGET_OBJECTS:bf_obj_fuzz>)
    target_include_directories(bf_lib_fuzz PUBLIC src)
    target_link_libraries(bf_lib_fuzz ${llvm_libs})
    target_compile_definitions(bf_lib_fuzz PUBLIC LLVM_AVAILABLE)
    target_link_options(bf_lib_fuzz PRIVATE -fsanitize=address,undefined -pie)

    add_executable(bfc_fuzz src/main_bfc.c)
    target_link_libraries(bfc_fuzz bf_lib_fuzz)
    set_target_properties(bfc_fuzz PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    target_compile_options(bfc_fuzz PRIVATE -fsanitize=address,undefined -g -O1 -fPIE)
    target_link_options(bfc_fuzz PRIVATE -fsanitize=address,undefined -pie)
    set_target_properties(bfc_fuzz PROPERTIES
        LINKER_LANGUAGE C
        LINK_EXECUTABLE "${AFL_CC} <OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${CMAKE_EXE_LINKER_FLAGS}"
    )
endif()

add_custom_target(fuzz
    COMMENT "Fuzzing with AFL, ASan, UBSan, and valgrind"
    COMMAND ${CMAKE_COMMAND} -E echo "--- Building with AFL and sanitizers ---"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target bfc_fuzz
    COMMAND ${CMAKE_COMMAND} -E echo "--- Copying only *.b files from test/fuzz to build directory ---"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/test/fuzz
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/test/fuzz/*.b ${CMAKE_BINARY_DIR}/test/fuzz/
    COMMAND ${CMAKE_COMMAND} -E echo "--- Running AFL fuzzer ---"
)

if(AFL_CC)
    add_custom_command(TARGET fuzz POST_BUILD
        COMMAND ${CMAKE_SOURCE_DIR}/docker/run_afl_parallel.sh ${CMAKE_BINARY_DIR}/bfc_fuzz
    )
endif()

find_program(VALGRIND NAMES valgrind)
if(NOT VALGRIND)
    message(WARNING "valgrind not found, valgrind step will be skipped in fuzz target.")
else()
    add_custom_command(TARGET fuzz POST_BUILD
        COMMAND ${VALGRIND} --leak-check=full --error-exitcode=1 ${CMAKE_BINARY_DIR}/bfc_fuzz < /dev/null || true
    )
endif()

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning all build files"
)
