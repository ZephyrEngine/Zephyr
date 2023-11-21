
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

  class VulkanComputePipeline final : public ComputePipeline {
    public:
     ~VulkanComputePipeline() override {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
      }

      void* Handle() override {
        return m_pipeline;
      }

      auto GetLayout() -> PipelineLayout* override {
        return m_layout.get();
      }

      static std::unique_ptr<VulkanComputePipeline> Create(
        VkDevice device,
        ShaderModule* shader_module,
        std::shared_ptr<PipelineLayout> layout
      ) {
        auto self = std::unique_ptr<VulkanComputePipeline>{new VulkanComputePipeline()};

        const VkComputePipelineCreateInfo info{
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

        if(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &self->m_pipeline) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanComputePipeline: failed to create the compute pipeline");
        }

        self->m_device = device;
        self->m_layout = std::move(layout);

        return self;
      }

    private:
      VulkanComputePipeline() = default;

      VkDevice m_device{VK_NULL_HANDLE};
      VkPipeline m_pipeline{VK_NULL_HANDLE};
      std::shared_ptr<PipelineLayout> m_layout{};
  };

} // namespace zephyr
