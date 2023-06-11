
cmake_minimum_required(VERSION 3.2)

project(zephyr-runtime CXX)

set(SOURCES
  src/main.cpp
)

set(HEADERS
)

add_executable(zephyr-runtime ${SOURCES} ${HEADERS})

target_compile_features(zephyr-runtime PRIVATE cxx_std_20)
target_include_directories(zephyr-runtime PRIVATE src)
target_link_libraries(zephyr-runtime PRIVATE zephyr)