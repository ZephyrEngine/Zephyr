
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

  class VulkanCommandPool final : public CommandPool {
    public:
      VulkanCommandPool(VkDevice device, u32 queue_family_index, Usage usage) : m_device{device} {
        const VkCommandPoolCreateInfo info{
          .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
          .pNext = nullptr,
          .flags = (VkCommandPoolCreateFlags)usage,
          .queueFamilyIndex = queue_family_index
        };

        if(vkCreateCommandPool(device, &info, nullptr, &m_pool) != VK_SUCCESS) {
          ZEPHYR_PANIC("Vulkan: failed to create a command pool");
        }
      }

     ~VulkanCommandPool() override {
        vkDestroyCommandPool(m_device, m_pool, nullptr);
      }

      void* Handle() override {
        return (void*)m_pool;
      }

    private:
      VkDevice m_device;
      VkCommandPool m_pool;
  };

} // namespace zephyr
