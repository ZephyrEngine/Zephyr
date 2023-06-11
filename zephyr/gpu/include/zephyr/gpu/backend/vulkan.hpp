
// Copyright (C) 2022 fleroviux. All rights reserved.

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

auto CreateVulkanRenderDevice(
  VulkanRenderDeviceOptions const& options
) -> std::unique_ptr<RenderDevice>;

} // namespace zephyr
