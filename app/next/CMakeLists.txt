
set(SOURCES
  src/renderer2/vulkan/vulkan_instance.cpp
  src/renderer2/render_engine.cpp
  src/main.cpp
  src/main_window.cpp
)

set(HEADERS
  src/renderer2/backend/vulkan/render_backend.cpp
  src/shader/triangle.frag.h
  src/shader/triangle.vert.h
  src/main_window.hpp
)

set(HEADERS_PUBLIC
  include/zephyr/renderer2/backend/render_backend.hpp
  include/zephyr/renderer2/backend/render_backend_vk.hpp
  include/zephyr/renderer2/vulkan/vulkan_instance.hpp
  include/zephyr/renderer2/vulkan/src/vulkan_physical_device.hpp
  include/zephyr/renderer2/render_engine.hpp
)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(zephyr-next ${SOURCES} ${HEADERS})

target_link_libraries(zephyr-next PRIVATE zephyr zephyr-cxx-opts SDL2::SDL2 Vulkan::Vulkan)
target_include_directories(zephyr-next PRIVATE include)
target_include_directories(zephyr-next PRIVATE src)
