
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

struct VulkanBindGroup final : BindGroup {
  VulkanBindGroup(
    VkDevice device,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout layout
  )   : device(device), descriptor_pool(descriptor_pool) {
    auto info = VkDescriptorSetAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout
    };

    if (vkAllocateDescriptorSets(device, &info, &descriptor_set) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanBindGroup: failed to allocate descriptor set");
    }
  }

 ~VulkanBindGroup() override {
    vkFreeDescriptorSets(device, descriptor_pool, 1, &descriptor_set);
  }

  auto Handle() -> void* override {
    return (void*)descriptor_set;
  }

  void Bind(
    u32 binding,
    Buffer* buffer,
    BindGroupLayout::Entry::Type type
  ) override {
    const auto buffer_info = VkDescriptorBufferInfo{
      .buffer = (VkBuffer)buffer->Handle(),
      .offset = 0,
      .range = VK_WHOLE_SIZE
    };

    const auto write_descriptor_set = VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = descriptor_set,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = (VkDescriptorType)type,
      .pImageInfo = nullptr,
      .pBufferInfo = &buffer_info,
      .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
  }

  void Bind(
    u32 binding,
    Texture::View* texture_view,
    Sampler* sampler,
    Texture::Layout layout
  ) override {
    const auto image_info = VkDescriptorImageInfo{
      .sampler = (VkSampler)sampler->Handle(),
      .imageView = (VkImageView)texture_view->Handle(),
      .imageLayout = (VkImageLayout)layout
    };

    const auto write_descriptor_set = VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = descriptor_set,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
  }

  void Bind(
    u32 binding,
    Texture::View* texture_view,
    Texture::Layout layout,
    BindGroupLayout::Entry::Type type
  ) override {
    const auto image_info = VkDescriptorImageInfo{
      .sampler = VK_NULL_HANDLE,
      .imageView = (VkImageView)texture_view->Handle(),
      .imageLayout = (VkImageLayout)layout
    };

    const auto write_descriptor_set = VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = descriptor_set,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = (VkDescriptorType)type,
      .pImageInfo = &image_info,
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
  }

private:
  VkDevice device;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_set;
};

struct VulkanBindGroupLayout final : BindGroupLayout {
  VulkanBindGroupLayout(
    VkDevice device,
    VkDescriptorPool descriptor_pool,
    std::span<BindGroupLayout::Entry const> entries
  ) : device(device), descriptor_pool(descriptor_pool) {
    auto bindings = std::vector<VkDescriptorSetLayoutBinding>{};

    for (auto const& entry : entries) {
      bindings.push_back({
        .binding = entry.binding,
        .descriptorType = (VkDescriptorType)entry.type,
        .descriptorCount = 1,
        .stageFlags = (VkShaderStageFlags)entry.stages,
        .pImmutableSamplers = nullptr
      });
    }

    auto info = VkDescriptorSetLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = (u32)bindings.size(),
      .pBindings = bindings.data()
    };

    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &layout) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanBindGroupLayout: failed to create descriptor set layout");
    }
  }

 ~VulkanBindGroupLayout() override {
    vkDestroyDescriptorSetLayout(device, layout, nullptr);
  }

  auto Handle() -> void* override {
   return (void*)layout;
  }

  auto Instantiate() -> std::unique_ptr<BindGroup> override {
    return std::make_unique<VulkanBindGroup>(device, descriptor_pool, layout);
  }

private:
  VkDevice device;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout layout;
};

} // namespace zephyr
