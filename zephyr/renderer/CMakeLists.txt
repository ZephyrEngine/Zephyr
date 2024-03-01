
set(SOURCES
  src/render_engine.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/renderer/buffer/buffer_resource.hpp
  include/zephyr/renderer/buffer/buffer_resource_base.hpp
  include/zephyr/renderer/buffer/index_buffer.hpp
  include/zephyr/renderer/buffer/uniform_buffer.hpp
  include/zephyr/renderer/buffer/vertex_buffer.hpp
  include/zephyr/renderer/glsl/type.hpp
  include/zephyr/renderer/glsl/std430_buffer_layout.hpp
  include/zephyr/renderer/glsl/variable_list.hpp
  include/zephyr/renderer/material/technique.hpp
  include/zephyr/renderer/texture/sampler_resource.hpp
  include/zephyr/renderer/texture/texture_2D.hpp
  include/zephyr/renderer/texture/texture_cube.hpp
  include/zephyr/renderer/texture/texture_resource.hpp
  include/zephyr/renderer/mesh.hpp
  include/zephyr/renderer/render_engine.hpp
  include/zephyr/renderer/resource.hpp
)

add_library(zephyr-renderer ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_link_libraries(zephyr-renderer PUBLIC zephyr-common zephyr-math zephyr-gpu zephyr-scene zephyr-cxx-opts)
target_include_directories(zephyr-renderer PUBLIC include)
target_include_directories(zephyr-renderer PRIVATE src)