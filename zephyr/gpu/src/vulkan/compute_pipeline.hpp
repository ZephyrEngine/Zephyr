
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

struct VulkanComputePipeline final : ComputePipeline {
 ~VulkanComputePipeline() override {
  }

  auto Handle() -> void* override {
    return pipeline;
  }

  auto GetLayout() -> PipelineLayout* override {
    return layout.get();
  }

  static auto Create(
    VkDevice device,
    ShaderModule* shader_module,
    std::shared_ptr<PipelineLayout> layout
  ) -> std::unique_ptr<VulkanComputePipeline> {
    auto self = std::unique_ptr<VulkanComputePipeline>{new VulkanComputePipeline()};

    const auto info = VkComputePipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = (VkShaderModule)shader_module->Handle(),
        .pName = "main",
        .pSpecializationInfo = nullptr
      },
      .layout = (VkPipelineLayout)layout->Handle(),
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0
    };

    if(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &self->pipeline) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanComputePipeline: failed to create the compute pipeline");
    }

    self->device = device;
    self->layout = std::move(layout);

    return self;
  }

private:
  VulkanComputePipeline() = default;

  VkDevice device = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  std::shared_ptr<PipelineLayout> layout{};
};

} // namespace zephyr
