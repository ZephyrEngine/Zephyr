
set(SOURCES
  src/dummy.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/scene/scene.hpp
)

add_library(zephyr-scene ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-scene PRIVATE src)
target_include_directories(zephyr-scene PUBLIC include)
target_link_libraries(zephyr-scene PUBLIC zephyr-common zephyr-cxx-opts)
