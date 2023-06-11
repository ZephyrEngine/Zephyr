cmake_minimum_required(VERSION 3.2)

project(zephyr-gpu CXX)

set(SOURCES
  src/vulkan/render_device.cpp
)

set(HEADERS
  src/vulkan/bind_group.hpp
  src/vulkan/buffer.hpp
  src/vulkan/command_buffer.hpp
  src/vulkan/command_pool.hpp
  src/vulkan/compute_pipeline.hpp
  src/vulkan/fence.hpp
  src/vulkan/pipeline_builder.hpp
  src/vulkan/pipeline_layout.hpp
  src/vulkan/queue.hpp
  src/vulkan/render_pass.hpp
  src/vulkan/render_pass_builder.hpp
  src/vulkan/render_target.hpp
  src/vulkan/sampler.hpp
  src/vulkan/shader_module.hpp
  src/vulkan/texture.hpp
  src/vulkan/texture_view.hpp
)

set(HEADERS_PUBLIC
  include/zephyr/gpu/backend/vulkan.hpp
  include/zephyr/gpu/bind_group.hpp
  include/zephyr/gpu/buffer.hpp
  include/zephyr/gpu/command_buffer.hpp
  include/zephyr/gpu/command_pool.hpp
  include/zephyr/gpu/compute_pipeline.hpp
  include/zephyr/gpu/enums.hpp
  include/zephyr/gpu/fence.hpp
  include/zephyr/gpu/pipeline_builder.hpp
  include/zephyr/gpu/pipeline_layout.hpp
  include/zephyr/gpu/queue.hpp
  include/zephyr/gpu/render_device.hpp
  include/zephyr/gpu/render_pass.hpp
  include/zephyr/gpu/render_target.hpp
  include/zephyr/gpu/sampler.hpp
  include/zephyr/gpu/shader_module.hpp
  include/zephyr/gpu/texture.hpp
)

find_package(Vulkan REQUIRED)

add_library(zephyr-gpu ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_compile_features(zephyr-gpu PUBLIC cxx_std_20)
target_link_libraries(zephyr-gpu PRIVATE Vulkan::Vulkan VulkanMemoryAllocator)
target_link_libraries(zephyr-gpu PUBLIC zephyr-common zephyr-logger)
target_include_directories(zephyr-gpu PUBLIC include)
target_include_directories(zephyr-gpu PRIVATE src)
