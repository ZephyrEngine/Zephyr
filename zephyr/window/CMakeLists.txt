
set(SOURCES
  src/vulkan/vulkan_swap_chain.cpp
  src/vulkan/vulkan_window.cpp
  src/window.cpp
)

set(HEADERS
  src/vulkan/vulkan_swap_chain.hpp
  src/vulkan/vulkan_window.hpp
  src/window.hpp
)

set(HEADERS_PUBLIC
  include/zephyr/window/swap_chain.hpp
  include/zephyr/window/window.hpp
)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_library(zephyr-window ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-window PRIVATE src)
target_include_directories(zephyr-window PUBLIC include)
target_link_libraries(zephyr-window PRIVATE SDL2::SDL2 Vulkan::Vulkan)
target_link_libraries(zephyr-window PUBLIC zephyr-gpu zephyr-cxx-opts)
