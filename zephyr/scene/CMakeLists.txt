
set(SOURCES
  src/transform.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/scene/scene.hpp
  include/zephyr/scene/transform.hpp
)

add_library(zephyr-scene ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-scene PRIVATE src)
target_include_directories(zephyr-scene PUBLIC include)
target_link_libraries(zephyr-scene PUBLIC zephyr-common zephyr-math zephyr-cxx-opts)
