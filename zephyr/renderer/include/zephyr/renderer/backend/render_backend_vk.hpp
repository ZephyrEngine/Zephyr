
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/vulkan/vulkan_instance.hpp>
#include <zephyr/integer.hpp>
#include <memory>
#include <vulkan/vulkan.h>
#include <vector>

namespace zephyr {

  struct VulkanRenderBackendProps {
    std::shared_ptr<VulkanInstance> vk_instance;
    VkSurfaceKHR vk_surface;
  };

  std::unique_ptr<RenderBackend> CreateVulkanRenderBackend(const VulkanRenderBackendProps& props);

} // namespace zephyr