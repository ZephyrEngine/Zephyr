
set(SOURCES
  src/placeholder.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/renderer/geometry/index_buffer.hpp
  include/zephyr/renderer/geometry/vertex_buffer.hpp
  include/zephyr/renderer/buffer_resource.hpp
  include/zephyr/renderer/resource.hpp
)

add_library(zephyr-renderer ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_link_libraries(zephyr-renderer PUBLIC zephyr-common zephyr-cxx-opts)
target_include_directories(zephyr-renderer PUBLIC include)
target_include_directories(zephyr-renderer PRIVATE src)