
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/sink/console.cpp
  src/sink/file.cpp
  src/logger.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/logger/sink/console.hpp
  include/zephyr/logger/sink/file.hpp
  include/zephyr/logger/logger.hpp
)

add_library(zephyr-logger ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_compile_features(zephyr-logger PUBLIC cxx_std_20)
target_link_libraries(zephyr-logger PUBLIC fmt zephyr-common)
target_include_directories(zephyr-logger PRIVATE src)
target_include_directories(zephyr-logger PUBLIC include)
