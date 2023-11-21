
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

  class VulkanBindGroup final : public BindGroup {
    public:
      VulkanBindGroup(
        VkDevice device,
        VkDescriptorPool descriptor_pool,
        const std::shared_ptr<BindGroupLayout>& layout
      )   : m_device{device}
          , m_descriptor_pool{descriptor_pool}
          , m_layout{layout} {
        const void* layout_handle = layout->Handle();

        const VkDescriptorSetAllocateInfo info{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .pNext = nullptr,
          .descriptorPool = descriptor_pool,
          .descriptorSetCount = 1,
          .pSetLayouts = (const VkDescriptorSetLayout*)&layout_handle
        };

        if (vkAllocateDescriptorSets(device, &info, &m_descriptor_set) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanBindGroup: failed to allocate descriptor set");
        }
      }

     ~VulkanBindGroup() override {
        vkFreeDescriptorSets(m_device, m_descriptor_pool, 1, &m_descriptor_set);
      }

      void* Handle() override {
        return (void*)m_descriptor_set;
      }

      void Bind(u32 binding, Buffer* buffer, BindingType type) override {
        const VkDescriptorBufferInfo buffer_info{
          .buffer = (VkBuffer)buffer->Handle(),
          .offset = 0,
          .range = VK_WHOLE_SIZE
        };

        const VkWriteDescriptorSet write_descriptor_set{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .pNext = nullptr,
          .dstSet = m_descriptor_set,
          .dstBinding = binding,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = (VkDescriptorType)type,
          .pImageInfo = nullptr,
          .pBufferInfo = &buffer_info,
          .pTexelBufferView = nullptr
        };

        vkUpdateDescriptorSets(m_device, 1, &write_descriptor_set, 0, nullptr);
      }

      void Bind(
        u32 binding,
        Texture::View* texture_view,
        Sampler* sampler,
        Texture::Layout layout
      ) override {
        const VkDescriptorImageInfo image_info{
          .sampler = (VkSampler)sampler->Handle(),
          .imageView = (VkImageView)texture_view->Handle(),
          .imageLayout = (VkImageLayout)layout
        };

        const VkWriteDescriptorSet write_descriptor_set{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .pNext = nullptr,
          .dstSet = m_descriptor_set,
          .dstBinding = binding,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &image_info,
          .pBufferInfo = nullptr,
          .pTexelBufferView = nullptr
        };

        vkUpdateDescriptorSets(m_device, 1, &write_descriptor_set, 0, nullptr);
      }

      void Bind(
        u32 binding,
        Texture::View* texture_view,
        Texture::Layout layout,
        BindingType type
      ) override {
        const VkDescriptorImageInfo image_info{
          .sampler = VK_NULL_HANDLE,
          .imageView = (VkImageView)texture_view->Handle(),
          .imageLayout = (VkImageLayout)layout
        };

        const VkWriteDescriptorSet write_descriptor_set{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .pNext = nullptr,
          .dstSet = m_descriptor_set,
          .dstBinding = binding,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = (VkDescriptorType)type,
          .pImageInfo = &image_info,
          .pBufferInfo = nullptr,
          .pTexelBufferView = nullptr
        };

        vkUpdateDescriptorSets(m_device, 1, &write_descriptor_set, 0, nullptr);
      }

    private:
      VkDevice m_device;
      VkDescriptorPool m_descriptor_pool;
      VkDescriptorSet m_descriptor_set{};
      std::shared_ptr<BindGroupLayout> m_layout;
  };

  class VulkanBindGroupLayout final : public BindGroupLayout {
    public:
      VulkanBindGroupLayout(
        VkDevice device,
        VkDescriptorPool descriptor_pool,
        std::span<BindGroupLayout::Entry const> entries
      )   : m_device{device}
          , m_descriptor_pool{descriptor_pool} {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        for (const auto& entry : entries) {
          bindings.push_back({
            .binding = entry.binding,
            .descriptorType = (VkDescriptorType)entry.type,
            .descriptorCount = 1,
            .stageFlags = (VkShaderStageFlags)entry.stages,
            .pImmutableSamplers = nullptr
          });
        }

        const VkDescriptorSetLayoutCreateInfo info{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .bindingCount = (u32)bindings.size(),
          .pBindings = bindings.data()
        };

        if (vkCreateDescriptorSetLayout(device, &info, nullptr, &m_layout) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanBindGroupLayout: failed to create descriptor set layout");
        }
      }

     ~VulkanBindGroupLayout() override {
        vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
      }

      void* Handle() override {
       return (void*)m_layout;
      }

    private:
      VkDevice m_device;
      VkDescriptorPool m_descriptor_pool;
      VkDescriptorSetLayout m_layout;
  };

} // namespace zephyr
