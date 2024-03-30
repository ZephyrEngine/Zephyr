
#pragma once

#include <zephyr/renderer2/backend/render_backend.hpp>
#include <zephyr/integer.hpp>
#include <memory>
#include <vulkan/vulkan.h>
#include <vector>

namespace zephyr {

  struct VulkanRenderBackendProps {
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphics_compute_queue;
    std::vector<u32> present_queue_family_indices;
  };

  std::unique_ptr<RenderBackend> CreateVulkanRenderBackend(const VulkanRenderBackendProps& props);

} // namespace zephyr