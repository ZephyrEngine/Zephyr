
set(SOURCES
  src/placeholder.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/renderer/resource/buffer_resource.hpp
  include/zephyr/renderer/resource/index_buffer.hpp
  include/zephyr/renderer/resource/resource.hpp
  include/zephyr/renderer/resource/uniform_buffer.hpp
  include/zephyr/renderer/resource/vertex_buffer.hpp
  include/zephyr/renderer/geometry.hpp
)

add_library(zephyr-renderer ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_link_libraries(zephyr-renderer PUBLIC zephyr-common zephyr-cxx-opts)
target_include_directories(zephyr-renderer PUBLIC include)
target_include_directories(zephyr-renderer PRIVATE src)