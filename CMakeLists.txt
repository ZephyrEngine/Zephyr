
cmake_minimum_required(VERSION 3.2)

project(zephyr CXX)

add_subdirectory(external)
add_subdirectory(zephyr)

option(ZEPHYR_BUILD_TESTS "Build the Zephyr test suite" ON)
option(ZEPHYR_BUILD_RUNTIME "Build the Zephyr game runtime" ON)

if(ZEPHYR_BUILD_TESTS)
  add_subdirectory(test)
endif()

if(ZEPHYR_BUILD_RUNTIME)
  add_subdirectory(app/runtime)
endif()
