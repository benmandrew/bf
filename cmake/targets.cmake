if(LLVM_AVAILABLE)
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
    target_link_options(bfi PRIVATE
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
    target_link_options(bfc PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    )
    set_target_properties(bfc bf_lib
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()