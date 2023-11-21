
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

  class VulkanShaderModule final : public ShaderModule {
    public:
      VulkanShaderModule(
        VkDevice device,
        u32 const* spirv,
        size_t size
      )   : device(device) {
        const VkShaderModuleCreateInfo info{
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .codeSize = size,
          .pCode = spirv
        };

        if (vkCreateShaderModule(device, &info, nullptr, &shader_module) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanShaderModule: failed to create shader module");
        }
      }

     ~VulkanShaderModule() override {
        vkDestroyShaderModule(device, shader_module, nullptr);
      }

      void* Handle() override {
        return (void*)shader_module;
      }

    private:
      VkDevice device;
      VkShaderModule shader_module;
  };

} // namespace zephyr
