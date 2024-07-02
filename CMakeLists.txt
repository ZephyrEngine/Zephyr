
cmake_minimum_required(VERSION 3.2)

project(zephyr CXX)

# Link against zephyr-cxx-opts to add common compiler flags to strictly our own targets.
add_library(zephyr-cxx-opts INTERFACE)
target_compile_features(zephyr-cxx-opts INTERFACE cxx_std_20)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # TODO: fix warnings, so that we can actually enable -Wall and -Wextra.
  # target_compile_options(zephyr-cxx-opts INTERFACE -Wall -Wextra -fno-exceptions)

  # SPIRV-Cross wants exceptions... even when we tell it to convert to asserts instead.
  # target_compile_options(zephyr-cxx-opts INTERFACE -fno-exceptions)
endif()

add_subdirectory(external)
add_subdirectory(zephyr)

option(ZEPHYR_BUILD_NEXT "Build Zephyr Next" ON)

if(ZEPHYR_BUILD_NEXT)
  add_subdirectory(app/next)
endif()