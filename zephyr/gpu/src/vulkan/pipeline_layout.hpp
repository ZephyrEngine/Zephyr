
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>

namespace zephyr {

struct VulkanPipelineLayout final : PipelineLayout {
  VulkanPipelineLayout(
    VkDevice device,
    std::span<BindGroupLayout* const> bind_group_layouts
  )   : device(device) {
    auto descriptor_set_layouts = std::vector<VkDescriptorSetLayout>{};

    for (auto& bind_group_layout : bind_group_layouts) {
      descriptor_set_layouts.push_back((VkDescriptorSetLayout)bind_group_layout->Handle());
    }

    const auto push_constant_rage = VkPushConstantRange{
      .stageFlags = VK_SHADER_STAGE_ALL,
      .offset = 0,
      .size = 128
    };

    const auto info = VkPipelineLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = (u32)descriptor_set_layouts.size(),
      .pSetLayouts = descriptor_set_layouts.data(),
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constant_rage
    };

    if (vkCreatePipelineLayout(device, &info, nullptr, &layout) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanPipelineLayout: failed to create pipeline layout");
    }
  }

 ~VulkanPipelineLayout() override {
    vkDestroyPipelineLayout(device, layout, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)layout;
  }

private:
  VkDevice device;
  VkPipelineLayout layout;
};

} // namespace zephyr
