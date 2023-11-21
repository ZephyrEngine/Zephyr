
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
    )   : allocator(allocator), size(size), host_visible(false) {
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

      host_visible = flags & Buffer::Flags::HostVisible;

      if(host_visible) {
        alloc_info.flags = flags & Buffer::Flags::HostAccessRandom ?
          VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT :
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
      }

      if(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr) != VK_SUCCESS) {
        ZEPHYR_PANIC("VulkanBuffer: failed to create buffer");
      }
    }

   ~VulkanBuffer() override {
      Unmap();
      vmaDestroyBuffer(allocator, buffer, allocation);
    }

    void* Handle() override {
      return (void*)buffer;
    }

    void Map() override {
      if(host_data == nullptr) {
        if(!host_visible) {
          ZEPHYR_PANIC("VulkanBuffer: attempted to map buffer which is not host visible");
        }

        if(vmaMapMemory(allocator, allocation, &host_data) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanBuffer: failed to map buffer to host memory, size={}", size);
        }
      }
    }

    void Unmap() override {
      if(host_data != nullptr) {
        vmaUnmapMemory(allocator, allocation);
        host_data = nullptr;
      }
    }

    void* Data() override {
      return host_data;
    }

    size_t Size() const override {
      return size;
    }

    void Flush() override {
      Flush(0, size);
    }

    void Flush(size_t offset, size_t size) override {
      auto range_end = offset + size;

      if(range_end > this->size) {
        ZEPHYR_PANIC("VulkanBuffer: out-of-bounds flush request, offset={}, size={}", offset, size);
      }

      if(vmaFlushAllocation(allocator, allocation, offset, size) != VK_SUCCESS) {
        ZEPHYR_PANIC("VulkanBuffer: failed to flush range");
      }
    }

  private:
    VkBuffer buffer;
    VmaAllocator allocator;
    VmaAllocation allocation;
    size_t size;
    bool host_visible;
    void* host_data = nullptr;
};

} // namespace zephyr
