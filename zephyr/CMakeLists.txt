
cmake_minimum_required(VERSION 3.2)

project(zephyr-core CXX)

add_subdirectory(common)
add_subdirectory(gpu)
add_subdirectory(logger)
add_subdirectory(math)

add_library(zephyr INTERFACE)

target_link_libraries(zephyr INTERFACE zephyr-common zephyr-gpu zephyr-logger zephyr-math)