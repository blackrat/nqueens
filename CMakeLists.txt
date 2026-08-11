# Copyright (c) 1992-2026 Paul McKibbin
cmake_minimum_required(VERSION 3.20)

project(nqueens
        VERSION 2.0.0
        DESCRIPTION "N-Queens by exhaustive backtracking or genetic search"
        LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

option(NQUEENS_BUILD_TESTS "Build the test suite" ON)
option(NQUEENS_WERROR "Treat warnings as errors" OFF)
option(NQUEENS_SANITIZE "Build with the address and undefined-behaviour sanitizers" OFF)

add_library(nqueens_warnings INTERFACE)
if(MSVC)
  target_compile_options(nqueens_warnings INTERFACE /W4 $<$<BOOL:${NQUEENS_WERROR}>:/WX>)
else()
  target_compile_options(nqueens_warnings INTERFACE
      -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes
      -Wconversion -Wsign-conversion -Wcast-qual -Wpointer-arith
      $<$<BOOL:${NQUEENS_WERROR}>:-Werror>)
endif()

if(NQUEENS_SANITIZE AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(nqueens_core STATIC
    src/backtrack.c
    src/board.c
    src/genetic.c
    src/keys.c
    src/options.c
    src/recorder.c
    src/runtime.c)
target_include_directories(nqueens_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(nqueens_core PRIVATE nqueens_warnings)

add_executable(nqueens src/main.c)
target_link_libraries(nqueens PRIVATE nqueens_core nqueens_warnings)

include(GNUInstallDirs)
install(TARGETS nqueens RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(NQUEENS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
