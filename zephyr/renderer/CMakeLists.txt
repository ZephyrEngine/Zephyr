
# TODO: split this up into multiple libraries in the future (i.e. one core library and one library per backend)
# TODO: remove dependency on SDL2 if possible

set(SOURCES
  src/backend/vulkan/render_backend.cpp
  src/engine/geometry_cache.cpp
  src/engine/material_cache.cpp
  src/engine/texture_cache.cpp
  src/vulkan/vulkan_instance.cpp
  src/render_engine.cpp
  src/render_scene.cpp
)

set(HEADERS
  src/backend/vulkan/shader/triangle.frag.h
  src/backend/vulkan/shader/triangle.vert.h
)

set(HEADERS_PUBLIC
  include/zephyr/renderer/backend/render_backend.hpp
  include/zephyr/renderer/backend/render_backend_vk.hpp
  include/zephyr/renderer/component/camera.hpp
  include/zephyr/renderer/component/mesh.hpp
  include/zephyr/renderer/engine/geometry_cache.hpp
  include/zephyr/renderer/engine/material_cache.hpp
  include/zephyr/renderer/engine/texture_cache.hpp
  include/zephyr/renderer/glsl/std140_buffer_layout.hpp
  include/zephyr/renderer/glsl/std430_buffer_layout.hpp
  include/zephyr/renderer/glsl/type.hpp
  include/zephyr/renderer/glsl/variable_list.hpp
  include/zephyr/renderer/resource/geometry.hpp
  include/zephyr/renderer/resource/material.hpp
  include/zephyr/renderer/resource/resource.hpp
  include/zephyr/renderer/resource/texture.hpp
  include/zephyr/renderer/resource/texture_2d.hpp
  include/zephyr/renderer/vulkan/vulkan_instance.hpp
  include/zephyr/renderer/vulkan/vulkan_physical_device.hpp
  include/zephyr/renderer/render_engine.hpp
  include/zephyr/renderer/render_scene.hpp
)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

if(ZEPHYR_ENABLE_OGL)
  list(APPEND SOURCES
    src/backend/opengl/render_geometry/render_geometry.cpp
    src/backend/opengl/render_geometry/render_geometry_manager.cpp
    src/backend/opengl/render_texture/render_texture.cpp
    src/backend/opengl/render_texture/render_texture_manager.cpp
    src/backend/opengl/dynamic_gpu_array.cpp
    src/backend/opengl/render_backend.cpp
  )

  list(APPEND HEADERS
    src/backend/opengl/render_geometry/render_geometry.hpp
    src/backend/opengl/render_geometry/render_geometry_manager.hpp
    src/backend/opengl/render_texture/render_texture.hpp
    src/backend/opengl/render_texture/render_texture_manager.hpp
    src/backend/opengl/shader/draw_call.glsl.hpp
    src/backend/opengl/shader/draw_list_builder.glsl.hpp
    src/backend/opengl/dynamic_gpu_array.hpp
    src/backend/opengl/render_backend.hpp
  )

  list(APPEND HEADERS_PUBLIC
    include/zephyr/renderer/backend/render_backend_ogl.hpp
  )

  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
endif()

add_library(zephyr-renderer ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-renderer PUBLIC include)
target_include_directories(zephyr-renderer PRIVATE src)
target_link_libraries(zephyr-renderer PUBLIC zephyr-common zephyr-logger zephyr-math zephyr-scene zephyr-cxx-opts SDL2::SDL2 Vulkan::Vulkan)

if(ZEPHYR_ENABLE_OGL)
  target_compile_definitions(zephyr-renderer PUBLIC ZEPHYR_OPENGL=1)
  target_link_libraries(zephyr-renderer PUBLIC OpenGL::GL GLEW::GLEW)
endif()
