
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>
#include <vk_mem_alloc.h>

#include "command_buffer.hpp"

namespace zephyr {

class VulkanBuffer final : public Buffer {
  public:
    VulkanBuffer(
      VmaAllocator allocator,
      Buffer::Usage usage,
      Buffer::Flags flags,
      size_t size
    )   : m_allocator{allocator}
        , m_size{size} {
      const VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = (VkBufferUsageFlags)(u16)usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
      };

      VmaAllocationCreateInfo alloc_info{
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO
      };

      m_host_visible = flags & Buffer::Flags::HostVisible;

      if(m_host_visible) {
        alloc_info.flags = flags & Buffer::Flags::HostAccessRandom ?
          VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT :
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
      }

      if(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &m_buffer, &m_allocation, nullptr) != VK_SUCCESS) {
        ZEPHYR_PANIC("VulkanBuffer: failed to create buffer");
      }
    }

   ~VulkanBuffer() override {
      Unmap();
      vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }

    void* Handle() override {
      return (void*)m_buffer;
    }

    void Map() override {
      if(m_host_data == nullptr) {
        if(!m_host_visible) {
          ZEPHYR_PANIC("VulkanBuffer: attempted to map buffer which is not host visible");
        }

        if(vmaMapMemory(m_allocator, m_allocation, &m_host_data) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanBuffer: failed to map buffer to host memory, size={}", m_size);
        }
      }
    }

    void Unmap() override {
      if(m_host_data != nullptr) {
        vmaUnmapMemory(m_allocator, m_allocation);
        m_host_data = nullptr;
      }
    }

    void* Data() override {
      return m_host_data;
    }

    size_t Size() const override {
      return m_size;
    }

    void Flush() override {
      Flush(0, m_size);
    }

    void Flush(size_t offset, size_t size) override {
      auto range_end = offset + size;

      if(range_end > this->m_size) {
        ZEPHYR_PANIC("VulkanBuffer: out-of-bounds flush request, offset={}, size={}", offset, size);
      }

      if(vmaFlushAllocation(m_allocator, m_allocation, offset, size) != VK_SUCCESS) {
        ZEPHYR_PANIC("VulkanBuffer: failed to flush range");
      }
    }

  private:
    VkBuffer m_buffer;
    VmaAllocator m_allocator;
    VmaAllocation m_allocation;
    size_t m_size;
    bool m_host_visible{false};
    void* m_host_data{};
};

} // namespace zephyr
