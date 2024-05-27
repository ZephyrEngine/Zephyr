
set(SOURCES
  src/scene_graph.cpp
  src/transform.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/scene/component.hpp
  include/zephyr/scene/scene_graph.hpp
  include/zephyr/scene/node.hpp
  include/zephyr/scene/transform.hpp
)

add_library(zephyr-scene ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-scene PRIVATE src)
target_include_directories(zephyr-scene PUBLIC include)
target_link_libraries(zephyr-scene PUBLIC zephyr-common zephyr-math zephyr-cxx-opts)
