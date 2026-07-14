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

set(LLVM_AVAILABLE FALSE)

if(NOT LLVM_FOUND)
    message(WARNING "LLVM is not available. Skipping bf library and executable targets. Docs and formatting remain available.")
else()
    set(LLVM_AVAILABLE TRUE)

    set(CMAKE_C_STANDARD 17)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS ON)  # For gnu17

    # cfg_dot.cpp uses the LLVM C++ API; LLVM 16+ headers require C++17.
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

    llvm_map_components_to_libnames(llvm_libs support core irreader passes
        analysis)

    # Use an OBJECT library for shared sources to avoid flag leakage
    set(LIB_SOURCES
        src/interp.c
        src/ir.c
        src/read.c
        src/llvm.c
        src/cfg_dot.cpp
    )
endif()