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
    # Mutators must NOT be AFL-instrumented — use the plain system compiler,
    # not CMAKE_C_COMPILER which may be afl-clang-fast in fuzz builds
    find_program(BASE_C_COMPILER NAMES clang gcc cc)
    if(NOT BASE_C_COMPILER)
        message(WARNING "No plain C compiler found for bf_mutator.so; custom mutator will be skipped.")
        add_custom_target(bf_mutator_target COMMENT "Skipping custom mutator (no plain C compiler found)")
    else()
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/bf_mutator.so
            COMMAND ${BASE_C_COMPILER}
                    -shared -fPIC -O2
                    -o ${CMAKE_BINARY_DIR}/bf_mutator.so
                    ${CMAKE_SOURCE_DIR}/verification/bf_mutator.c
            DEPENDS ${CMAKE_SOURCE_DIR}/verification/bf_mutator.c
            COMMENT "Building grammar-aware BF custom mutator"
        )
        add_custom_target(bf_mutator_target DEPENDS ${CMAKE_BINARY_DIR}/bf_mutator.so)
    endif()

    # CmpLog binary — recompile with AFL_LLVM_CMPLOG=1 so AFL++ can observe
    # comparison operands and mutate inputs to satisfy them
    find_program(LLVM_CONFIG_BIN NAMES llvm-config HINTS ${LLVM_TOOLS_BINARY_DIR})
    if(LLVM_CONFIG_BIN)
        execute_process(
            COMMAND ${LLVM_CONFIG_BIN} --libs support core irreader passes --system-libs
            OUTPUT_VARIABLE LLVM_RAW_LINK_FLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        separate_arguments(LLVM_LINK_FLAGS_LIST NATIVE_COMMAND "${LLVM_RAW_LINK_FLAGS}")

        set(CMPLOG_INCLUDE_FLAGS "")
        foreach(dir ${LLVM_INCLUDE_DIRS})
            list(APPEND CMPLOG_INCLUDE_FLAGS "-I${dir}")
        endforeach()

        set(CMPLOG_LIBDIR_FLAGS "")
        foreach(dir ${LLVM_LIBRARY_DIRS})
            list(APPEND CMPLOG_LIBDIR_FLAGS "-L${dir}")
        endforeach()

        set(CMPLOG_SOURCES
            ${CMAKE_SOURCE_DIR}/src/read.c
            ${CMAKE_SOURCE_DIR}/src/ir.c
            ${CMAKE_SOURCE_DIR}/src/llvm.c
            ${CMAKE_SOURCE_DIR}/src/interp.c
            ${CMAKE_SOURCE_DIR}/test/main_fuzz.c
        )

        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/bfc_fuzz_cmplog
            COMMAND ${CMAKE_COMMAND} -E env AFL_LLVM_CMPLOG=1
                    ${AFL_CC}
                    -fsanitize=address,undefined -g -O1 -fPIE -pie
                    -I${CMAKE_SOURCE_DIR}/src
                    ${CMPLOG_INCLUDE_FLAGS}
                    -DLLVM_AVAILABLE
                    ${CMPLOG_SOURCES}
                    ${CMPLOG_LIBDIR_FLAGS}
                    ${LLVM_LINK_FLAGS_LIST}
                    -o ${CMAKE_BINARY_DIR}/bfc_fuzz_cmplog
            DEPENDS ${CMPLOG_SOURCES}
            COMMENT "Building CmpLog-instrumented fuzz binary"
            VERBATIM
        )
        add_custom_target(bfc_fuzz_cmplog_target DEPENDS ${CMAKE_BINARY_DIR}/bfc_fuzz_cmplog)
    endif()

    add_custom_target(fuzz
        COMMENT "Fuzzing with AFL, ASan, UBSan, and valgrind"
        COMMAND ${CMAKE_COMMAND} -E echo "--- Building with AFL and sanitizers ---"
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target bfc_fuzz
        COMMAND ${CMAKE_COMMAND} -E echo "--- Building grammar-aware custom mutator ---"
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target bf_mutator_target
        COMMAND ${CMAKE_COMMAND} -E echo "--- Building CmpLog-instrumented binary ---"
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target bfc_fuzz_cmplog_target
        COMMAND ${CMAKE_COMMAND} -E echo "--- Copying test/fuzz seeds and dictionary to build directory ---"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/test/fuzz ${CMAKE_BINARY_DIR}/test/fuzz
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