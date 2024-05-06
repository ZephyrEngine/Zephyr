
# TODO: split this up into multiple libraries in the future (i.e. one core library and one library per backend)
# TODO: remove dependency on SDL2 if possible

set(SOURCES
  src/backend/opengl/dynamic_gpu_array.cpp
  src/backend/opengl/render_backend.cpp
  src/backend/opengl/render_geometry.cpp
  src/backend/vulkan/render_backend.cpp
  src/engine/geometry_cache.cpp
  src/vulkan/vulkan_instance.cpp
  src/render_engine.cpp
)

set(HEADERS
  src/backend/opengl/dynamic_gpu_array.hpp
  src/backend/opengl/render_backend.hpp
  src/backend/opengl/render_geometry.hpp
  src/backend/vulkan/shader/triangle.frag.h
  src/backend/vulkan/shader/triangle.vert.h
)

set(HEADERS_PUBLIC
  include/zephyr/renderer/backend/render_backend.hpp
  include/zephyr/renderer/backend/render_backend_ogl.hpp
  include/zephyr/renderer/backend/render_backend_vk.hpp
  include/zephyr/renderer/component/camera.hpp
  include/zephyr/renderer/component/mesh.hpp
  include/zephyr/renderer/engine/geometry_cache.hpp
  include/zephyr/renderer/resource/geometry.hpp
  include/zephyr/renderer/resource/resource.hpp
  include/zephyr/renderer/vulkan/vulkan_instance.hpp
  include/zephyr/renderer/vulkan/vulkan_physical_device.hpp
  include/zephyr/renderer/render_engine.hpp
)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_library(zephyr-renderer ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-renderer PUBLIC include)
target_include_directories(zephyr-renderer PRIVATE src)
target_link_libraries(zephyr-renderer PUBLIC zephyr-common zephyr-logger zephyr-math zephyr-scene zephyr-cxx-opts SDL2::SDL2 Vulkan::Vulkan OpenGL::GL GLEW::GLEW)