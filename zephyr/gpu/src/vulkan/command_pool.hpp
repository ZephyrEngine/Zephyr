
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

struct VulkanCommandPool final : CommandPool {
  VulkanCommandPool(VkDevice device, u32 queue_family_index, Usage usage) : device(device) {
    auto info = VkCommandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = (VkCommandPoolCreateFlags)usage,
      .queueFamilyIndex = queue_family_index
    };

    if (vkCreateCommandPool(device, &info, nullptr, &pool) != VK_SUCCESS) {
      ZEPHYR_PANIC("Vulkan: failed to create a command pool");
    }
  }

 ~VulkanCommandPool() override {
    vkDestroyCommandPool(device, pool, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)pool;
  }

private:
  VkDevice device;
  VkCommandPool pool;
};

} // namespace zephyr
