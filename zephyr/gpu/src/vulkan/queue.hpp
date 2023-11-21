
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>

namespace zephyr {

  class VulkanQueue final : public Queue {
    public:
      explicit VulkanQueue(VkQueue queue) : m_queue{queue} {}

      void* Handle() override {
        return (void*)m_queue;
      }

      void Submit(std::span<CommandBuffer* const> buffers, Fence* fence) override {
        VkCommandBuffer handles[buffers.size()];

        for (size_t i = 0; i < buffers.size(); i++) {
          handles[i] = (VkCommandBuffer)buffers[i]->Handle();
        }

        const VkSubmitInfo submit{
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

        vkQueueSubmit(m_queue, 1, &submit, fence_handle);
      }

      void WaitIdle() override {
        vkQueueWaitIdle(m_queue);
      }

    private:
      VkQueue m_queue;
  };

} // namespace zephyr
