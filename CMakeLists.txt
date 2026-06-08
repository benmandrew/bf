cmake_minimum_required(VERSION 3.20)
project(bf VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for tooling" FORCE)

include(${CMAKE_SOURCE_DIR}/cmake/llvm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/targets.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/testing.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/formatting.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/linting.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/fuzzing.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/web.cmake)
