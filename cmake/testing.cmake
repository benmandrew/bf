option(DOCKER_BUILD "Docker build mode - disables tests and verification" OFF)
if(NOT DOCKER_BUILD AND LLVM_AVAILABLE)
    enable_testing()
    add_subdirectory(test)
    add_subdirectory(verification)
elseif(NOT LLVM_AVAILABLE AND NOT DOCKER_BUILD)
    message(WARNING "Skipping test and verification subdirectories because LLVM is not available.")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/docs")
    add_subdirectory(docs)
endif()

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Debug
    COMMENT "Building in debug mode"
)