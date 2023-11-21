
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

  class VulkanSampler final : public Sampler {
    public:
      VulkanSampler(VkDevice device, const Config& config) : device(device) {
        // @todo: clamp anisotropy level to the hardware-supported level.
        const VkSamplerCreateInfo info{
         .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
         .pNext = nullptr,
         .flags = 0,
         .magFilter = (VkFilter)config.mag_filter,
         .minFilter = (VkFilter)config.min_filter,
         .mipmapMode = (VkSamplerMipmapMode)config.mip_filter,
         .addressModeU = (VkSamplerAddressMode)config.address_mode_u,
         .addressModeV = (VkSamplerAddressMode)config.address_mode_v,
         .addressModeW = (VkSamplerAddressMode)config.address_mode_w,
         .mipLodBias = 0,
         .anisotropyEnable = config.anisotropy ? VK_TRUE : VK_FALSE,
         .maxAnisotropy = (float)config.max_anisotropy,
         .compareEnable = VK_FALSE,
         .compareOp = VK_COMPARE_OP_NEVER,
         .minLod = 0,
         .maxLod = VK_LOD_CLAMP_NONE,
         .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
         .unnormalizedCoordinates = VK_FALSE
        };

        if (vkCreateSampler(device, &info, nullptr, &sampler) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanSampler: failed to create sampler");
        }
      }

     ~VulkanSampler() override {
        vkDestroySampler(device, sampler, nullptr);
      }

      void* Handle() override {
        return (void*)sampler;
      }

    private:
      VkDevice device;
      VkSampler sampler;
  };

} // namespace zephyr
