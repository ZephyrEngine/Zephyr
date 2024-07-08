
# TODO: remove dependencies on OpenGL and SDL2

set(SOURCES
  src/backend/opengl/render_geometry/render_geometry.cpp
  src/backend/opengl/render_geometry/render_geometry_manager.cpp
  src/backend/opengl/dynamic_gpu_array.cpp
  src/backend/opengl/render_backend.cpp
  src/engine/geometry_cache.cpp
  src/render_engine.cpp
  src/render_scene.cpp
)

set(HEADERS
  src/backend/opengl/render_geometry/render_geometry.hpp
  src/backend/opengl/render_geometry/render_geometry_manager.hpp
  src/backend/opengl/shader/draw_call.glsl.hpp
  src/backend/opengl/shader/draw_list_builder.glsl.hpp
  src/backend/opengl/dynamic_gpu_array.hpp
  src/backend/opengl/render_backend.hpp
)

set(HEADERS_PUBLIC
  include/zephyr/renderer/backend/render_backend.hpp
  include/zephyr/renderer/backend/render_backend_ogl.hpp
  include/zephyr/renderer/component/camera.hpp
  include/zephyr/renderer/component/mesh.hpp
  include/zephyr/renderer/engine/geometry_cache.hpp
  include/zephyr/renderer/resource/geometry.hpp
  include/zephyr/renderer/resource/material.hpp
  include/zephyr/renderer/resource/resource.hpp
  include/zephyr/renderer/render_engine.hpp
  include/zephyr/renderer/render_scene.hpp
)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_library(zephyr-renderer ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-renderer PUBLIC include)
target_include_directories(zephyr-renderer PRIVATE src)
target_link_libraries(zephyr-renderer PUBLIC zephyr-common zephyr-logger zephyr-math zephyr-scene zephyr-cxx-opts SDL2::SDL2 OpenGL::GL GLEW::GLEW)