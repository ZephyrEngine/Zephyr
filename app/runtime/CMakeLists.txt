
set(SOURCES
  src/renderer/buffer_cache.cpp
  src/renderer/material_pipeline_cache.cpp
  src/renderer/resource_uploader.cpp
  src/renderer/sampler_cache.cpp
  src/renderer/texture_cache.cpp
  src/application.cpp
  src/main.cpp
  src/main_window.cpp
)

set(HEADERS
  src/renderer/buffer_cache.hpp
  src/renderer/material.hpp
  src/renderer/material_pipeline_cache.hpp
  src/renderer/resource_uploader.hpp
  src/renderer/sampler_cache.hpp
  src/renderer/std430_buffer_layout.hpp
  src/renderer/texture_cache.hpp
  src/shader/mesh.frag.h
  src/shader/mesh.vert.h
  src/application.hpp
  src/main_window.hpp
)

add_executable(zephyr-runtime ${SOURCES} ${HEADERS})

target_include_directories(zephyr-runtime PRIVATE src)
target_link_libraries(zephyr-runtime PRIVATE zephyr zephyr-cxx-opts stb glslang glslang-default-resource-limits SPIRV)