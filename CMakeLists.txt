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
                        "\n"
                        "Installation instructions:\n"
                        "\n"
                        "macOS (Homebrew):\n"
                        "  brew install llvm\n"
                        "\n"
                        "Ubuntu/Debian:\n"
                        "  sudo apt-get install llvm-dev\n"
                        "\n"
                        "Fedora/RHEL:\n"
                        "  sudo dnf install llvm-devel\n"
                        "\n"
                        "Or set LLVM_DIR manually:\n"
                        "  cmake -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm ..\n")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)  # For gnu17

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O2")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm_libs support core irreader)

set(LLVM_AVAILABLE TRUE)

set(LIB_SOURCES
    src/interp.c
    src/ir.c
    src/read.c
    src/llvm.c
)

add_library(bf_lib STATIC ${LIB_SOURCES})
target_include_directories(bf_lib PUBLIC src)

target_link_libraries(bf_lib ${llvm_libs})
target_compile_definitions(bf_lib PUBLIC LLVM_AVAILABLE)

add_executable(bf src/main.c)
target_link_libraries(bf bf_lib)

set_target_properties(bf bf_lib 
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_subdirectory(test)

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

# Lint target with clang-tidy
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    file(GLOB_RECURSE LINT_SOURCE_FILES "src/*.c")
    add_custom_target(lint
        COMMAND ${CLANG_TIDY} ${LINT_SOURCE_FILES}
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
            -- -I${CMAKE_SOURCE_DIR}/src ${LLVM_DEFINITIONS_LIST}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on source files"
    )
    add_custom_target(lint-fix
        COMMAND ${CLANG_TIDY} ${LINT_SOURCE_FILES}
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
            --fix
            -- -I${CMAKE_SOURCE_DIR}/src ${LLVM_DEFINITIONS_LIST}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy with auto-fix on source files"
    )
else()
    message(WARNING "clang-tidy not found, lint targets will not be available")
endif()

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning all build files"
)
