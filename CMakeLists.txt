cmake_minimum_required(VERSION 3.20)
project(bf VERSION 1.0.0 LANGUAGES C CXX)

# Find LLVM (optional)
# Add common LLVM installation paths for better detection
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

# First try to find LLVM without hints
find_package(LLVM CONFIG QUIET)

# If not found, try with path hints
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

# Add helpful installation message if LLVM not found
if(NOT LLVM_FOUND)
    message(STATUS "LLVM installation instructions:")
    message(STATUS "")
    message(STATUS "macOS (Homebrew):")
    message(STATUS "  brew install llvm")
    message(STATUS "")
    message(STATUS "Ubuntu/Debian:")
    message(STATUS "  sudo apt-get install llvm-dev")
    message(STATUS "")
    message(STATUS "Fedora/RHEL:")
    message(STATUS "  sudo dnf install llvm-devel")
    message(STATUS "")
    message(STATUS "Or set LLVM_DIR manually:")
    message(STATUS "  cmake -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm ..")
endif()

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)  # For gnu17

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O2")

if(LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
    
    # Set up LLVM
    include_directories(${LLVM_INCLUDE_DIRS})
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
    add_definitions(${LLVM_DEFINITIONS_LIST})
    
    # Find the LLVM libraries that correspond to the components we want to use
    llvm_map_components_to_libnames(llvm_libs support core irreader)
    
    set(LLVM_AVAILABLE TRUE)
else()
    message(WARNING "LLVM not found. Building without LLVM support.")
    message(STATUS "To enable LLVM support, install LLVM and set LLVM_DIR or CMAKE_PREFIX_PATH")
    set(LLVM_AVAILABLE FALSE)
    set(llvm_libs "")
endif()

# Source files (excluding main.c for library)
set(LIB_SOURCES
    src/interp.c
    src/ir.c
    src/read.c
)

# Create a library for shared code
add_library(bf_lib STATIC ${LIB_SOURCES})
target_include_directories(bf_lib PUBLIC src)

# Link LLVM libraries if available
if(LLVM_AVAILABLE)
    target_link_libraries(bf_lib ${llvm_libs})
    target_compile_definitions(bf_lib PUBLIC LLVM_AVAILABLE)
endif()

# Main executable
add_executable(bf src/main.c)
target_link_libraries(bf bf_lib)

# Set output directory
set_target_properties(bf bf_lib 
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Find Check framework for unit tests
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CHECK check)
endif()

if(CHECK_FOUND)
    # Test sources
    set(TEST_SOURCES
        test/test_main.c
        test/test_interp.c
        test/test_ir.c
    )

    # Test executable
    add_executable(test_runner ${TEST_SOURCES})
    target_link_libraries(test_runner bf_lib ${CHECK_LIBRARIES})
    target_include_directories(test_runner PRIVATE ${CHECK_INCLUDE_DIRS})
    target_compile_options(test_runner PRIVATE ${CHECK_CFLAGS_OTHER})
    target_link_directories(test_runner PRIVATE ${CHECK_LIBRARY_DIRS})

    # Unit tests target
    add_custom_target(unittest
        COMMAND ./test_runner
        DEPENDS test_runner
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests"
    )

    # Find expect for integration tests
    find_program(EXPECT expect)
    if(EXPECT)
        # Expect tests
        file(GLOB EXPECT_TESTS "test/test_*.exp")
        add_custom_target(expecttest
            COMMAND echo "Running all expect tests..."
            DEPENDS bf
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running expect tests"
        )
        
        foreach(test_file ${EXPECT_TESTS})
            get_filename_component(test_name ${test_file} NAME_WE)
            add_custom_command(TARGET expecttest POST_BUILD
                COMMAND echo "Running ${test_file}..."
                COMMAND ${EXPECT} ${test_file}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                VERBATIM
            )
        endforeach()

        # Combined test target
        add_custom_target(test
            DEPENDS unittest expecttest
            COMMENT "Running all tests"
        )
    else()
        message(WARNING "expect not found, expect tests will not be available")
        add_custom_target(test
            DEPENDS unittest
            COMMENT "Running unit tests only (expect not found)"
        )
    endif()
else()
    message(WARNING "Check framework not found, unit tests will not be built")
endif()

# Debug target (same as normal build with debug symbols)
add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Debug
    COMMENT "Building in debug mode"
)

# Format target
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

# Clean target (CMake provides this by default, but we can add a custom one)
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning all build files"
)
