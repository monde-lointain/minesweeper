cmake_minimum_required(VERSION 3.28)
project(minesweeper VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options. Tests default on only when we are the top-level project, so an
# embedding parent (add_subdirectory / FetchContent) does not build them.
option(BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(ENABLE_LTO "Enable Link-Time Optimization for Release builds" OFF)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Link-Time Optimization (off by default; clang LTO needs a matching toolchain)
if(ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
  if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "LTO enabled for Release builds")
  else()
    message(WARNING "LTO not supported: ${ipo_error}")
  endif()
endif()

# Sanitizers — carried by an INTERFACE target that src/ links PUBLIC into the
# project library, so only code we own (lib + executable + tests) is instrumented.
# The FetchContent dependencies never link this target, so they stay un-instrumented.
add_library(minesweeper_sanitizers INTERFACE)
if(ENABLE_SANITIZERS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(minesweeper_sanitizers INTERFACE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(minesweeper_sanitizers INTERFACE
      -fsanitize=address,undefined)
    message(STATUS "Sanitizers enabled (project targets only)")
  endif()
endif()

# cmake modules (trimmed: build + deps + orthodoxy + tidy + format)
include(cmake/Dependencies.cmake)
include(cmake/Orthodoxy.cmake)
include(cmake/StaticAnalysis.cmake)
include(cmake/Formatting.cmake)

add_subdirectory(src)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
