
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <vulkan/vulkan.h>

namespace zephyr {

  struct VulkanRenderDeviceOptions {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    u32 queue_family_graphics;
    u32 queue_family_compute;
  };

  std::unique_ptr<RenderDevice> CreateVulkanRenderDevice(const VulkanRenderDeviceOptions& options);

} // namespace zephyr
