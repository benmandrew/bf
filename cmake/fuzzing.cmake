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

    add_executable(bfc_fuzz test/main_fuzz.c)
    target_link_libraries(bfc_fuzz bf_lib_fuzz)
    set_target_properties(bfc_fuzz PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    target_compile_options(bfc_fuzz PRIVATE -fsanitize=address,undefined -g -O1 -fPIE)
    target_link_options(bfc_fuzz PRIVATE -fsanitize=address,undefined -pie)
    set_target_properties(bfc_fuzz PROPERTIES
        LINKER_LANGUAGE C
        LINK_EXECUTABLE "${AFL_CC} <OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${CMAKE_EXE_LINKER_FLAGS}"
    )
endif()

if(AFL_CC)
    add_custom_target(fuzz
        COMMENT "Fuzzing with AFL, ASan, UBSan, and valgrind"
        COMMAND ${CMAKE_COMMAND} -E echo "--- Building with AFL and sanitizers ---"
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target bfc_fuzz
        COMMAND ${CMAKE_COMMAND} -E echo "--- Copying only *.b files from test/fuzz to build directory ---"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/test/fuzz
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/test/fuzz/*.b ${CMAKE_BINARY_DIR}/test/fuzz/
        COMMAND ${CMAKE_COMMAND} -E echo "--- Running AFL fuzzer ---"
    )

    add_custom_command(TARGET fuzz POST_BUILD
        COMMAND ${CMAKE_SOURCE_DIR}/verification/run_afl_parallel.sh ${CMAKE_BINARY_DIR}/bfc_fuzz
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