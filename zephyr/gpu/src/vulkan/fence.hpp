
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

  class VulkanFence final : public Fence {
    public:
      VulkanFence(VkDevice device, CreateSignalled create_signalled) : m_device{device} {
        const VkFenceCreateInfo info{
          .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
          .pNext = nullptr,
          .flags = (create_signalled == CreateSignalled::Yes) ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
        };

        if(vkCreateFence(m_device, &info, nullptr, &m_fence) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanFence: failed to create fence :(");
        }
      }

     ~VulkanFence() override {
        vkDestroyFence(m_device, m_fence, nullptr);
      }

      void* Handle() override {
        return (void*)m_fence;
      }

      void Reset() override {
        vkResetFences(m_device, 1, &m_fence);
      }

      void Wait(u64 timeout_ns) override {
        vkWaitForFences(m_device, 1, &m_fence, VK_FALSE, timeout_ns);
      }

    private:
      VkDevice m_device;
      VkFence m_fence;
  };

} // namespace zephyr
