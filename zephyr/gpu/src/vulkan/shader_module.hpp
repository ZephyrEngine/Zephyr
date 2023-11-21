
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

  class VulkanShaderModule final : public ShaderModule {
    public:
      VulkanShaderModule(
        VkDevice device,
        const u32* spirv,
        size_t size
      )   : m_device{device} {
        const VkShaderModuleCreateInfo info{
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .codeSize = size,
          .pCode = spirv
        };

        if (vkCreateShaderModule(device, &info, nullptr, &m_shader_module) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanShaderModule: failed to create shader module");
        }
      }

     ~VulkanShaderModule() override {
        vkDestroyShaderModule(m_device, m_shader_module, nullptr);
      }

      void* Handle() override {
        return (void*)m_shader_module;
      }

    private:
      VkDevice m_device;
      VkShaderModule m_shader_module;
  };

} // namespace zephyr
