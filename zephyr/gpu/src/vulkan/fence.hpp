
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

struct VulkanFence final : Fence {
  VulkanFence(VkDevice device, CreateSignalled create_signalled) : device(device) {
    auto info = VkFenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = (create_signalled == CreateSignalled::Yes) ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
    };

    if (vkCreateFence(device, &info, nullptr, &fence) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanFence: failed to create fence :(");
    }
  }

 ~VulkanFence() override {
    vkDestroyFence(device, fence, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)fence;
  }

  void Reset() override {
    vkResetFences(device, 1, &fence);
  }

  void Wait(u64 timeout_ns) override {
    vkWaitForFences(device, 1, &fence, VK_FALSE, timeout_ns);
  }

private:
  VkDevice device;
  VkFence fence;
};

} // namespace zephyr
