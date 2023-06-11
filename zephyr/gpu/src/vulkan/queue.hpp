
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>

namespace zephyr {

struct VulkanQueue final : Queue {
  VulkanQueue(VkQueue queue) : queue(queue) {}

  auto Handle() -> void* override {
    return (void*)queue;
  }

  void Submit(std::span<CommandBuffer* const> buffers, Fence* fence) override {
    VkCommandBuffer handles[buffers.size()];

    for (size_t i = 0; i < buffers.size(); i++) {
      handles[i] = (VkCommandBuffer)buffers[i]->Handle();
    }

    const auto submit = VkSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = (u32)buffers.size(),
      .pCommandBuffers = handles,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
    };

    const auto fence_handle = fence ? (VkFence)fence->Handle() : VK_NULL_HANDLE;

    vkQueueSubmit(queue, 1, &submit, fence_handle);
  }

  void WaitIdle() override {
    vkQueueWaitIdle(queue);
  }

private:
  VkQueue queue;
};

} // namespace zephyr
