find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CHECK check)
endif()

if(CHECK_FOUND)
    set(TEST_SOURCES
        test_main.c
        test_interp.c
        test_ir.c
    )
    add_executable(test_runner ${TEST_SOURCES})
    target_link_libraries(test_runner bf_lib ${CHECK_LIBRARIES})
    target_include_directories(test_runner PRIVATE ${CHECK_INCLUDE_DIRS})
    target_compile_options(test_runner PRIVATE ${CHECK_CFLAGS_OTHER})
    target_link_directories(test_runner PRIVATE ${CHECK_LIBRARY_DIRS})
    add_custom_target(unittest
        COMMAND ./test/test_runner
        DEPENDS test_runner
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests"
    )
else()
    message(WARNING "Check framework not found, unit tests will not be built")
    add_custom_target(unittest
        COMMAND echo "Unit tests not available - Check framework not found"
        COMMENT "Unit tests disabled"
    )
endif()

find_program(EXPECT expect)
if(EXPECT)
    file(GLOB EXPECT_TESTS "test_*.exp")
    add_custom_target(expecttest
        COMMAND echo "Running all expect tests..."
        DEPENDS bfi
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
else()
    message(WARNING "expect not found, expect tests will not be available")
    add_custom_target(expecttest
        COMMAND echo "Expect tests not available - expect not found"
        COMMENT "Expect tests disabled"
    )
endif()

find_program(FILECHECK NAMES FileCheck llvm-filecheck filecheck PATHS /usr/bin /usr/lib/llvm-*/bin)
if(FILECHECK AND LLVM_AVAILABLE)
    message(STATUS "Found FileCheck at: ${FILECHECK}")
    file(GLOB FILECHECK_TESTS "*.filecheck")
    if(FILECHECK_TESTS)
        add_custom_target(filecheck
            COMMAND echo "Running all FileCheck tests..."
            DEPENDS bfc
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running FileCheck tests"
        )
        foreach(test_file ${FILECHECK_TESTS})
            get_filename_component(test_name ${test_file} NAME_WE)
            string(REGEX REPLACE "^test_" "" bf_name "${test_name}")
            set(input_file "${CMAKE_SOURCE_DIR}/test/res/${bf_name}.b")
            if(EXISTS ${input_file})
                add_custom_command(TARGET filecheck POST_BUILD
                    COMMAND echo "Running FileCheck test: ${test_name}"
                    COMMAND ${CMAKE_BINARY_DIR}/bfc ${input_file} | ${FILECHECK} ${test_file}
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    VERBATIM
                )
            else()
                message(WARNING "Input file not found for FileCheck test: ${input_file}")
            endif()
        endforeach()
    else()
        message(STATUS "No .filecheck files found in test/ directory")
        add_custom_target(filecheck
            COMMAND echo "FileCheck tests not available - no .filecheck files found"
            COMMENT "FileCheck tests disabled"
        )
    endif()
else()
    if(NOT FILECHECK)
        message(WARNING "FileCheck not found, FileCheck tests will not be available")
    endif()
    if(NOT LLVM_AVAILABLE)
        message(WARNING "LLVM not available, FileCheck tests will not be available")
    endif()
    add_custom_target(filecheck
        COMMAND echo "FileCheck tests not available - FileCheck not found or LLVM not available"
        COMMENT "FileCheck tests disabled"
    )
endif()

add_custom_target(tests
    DEPENDS unittest expecttest filecheck
    COMMENT "Running all test suites"
)

add_executable(main_fuzz main_fuzz.c)
target_link_libraries(main_fuzz bf_lib)
