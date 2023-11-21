
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/gpu/texture.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

  class VulkanTextureView final : public Texture::View {
    public:
      VulkanTextureView(
        VkDevice device,
        Texture* texture,
        Type type,
        Texture::Format format,
        u32 width,
        u32 height,
        Texture::SubresourceRange const& range,
        ComponentMapping const& mapping
      )   : device(device), type(type), format(format), width(width), height(height), range(range), mapping(mapping) {
        const VkImageViewCreateInfo info{
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .image = (VkImage)texture->Handle(),
          .viewType = (VkImageViewType)type,
          .format = (VkFormat)format,
          .components = VkComponentMapping{
            .r = (VkComponentSwizzle)mapping.r,
            .g = (VkComponentSwizzle)mapping.g,
            .b = (VkComponentSwizzle)mapping.b,
            .a = (VkComponentSwizzle)mapping.a,
          },
          .subresourceRange = VkImageSubresourceRange{
            .aspectMask = (VkImageAspectFlags)range.aspect,
            .baseMipLevel = range.base_mip,
            .levelCount = range.mip_count,
            .baseArrayLayer = range.base_layer,
            .layerCount = range.layer_count
          }
        };

        if (vkCreateImageView(device, &info, nullptr, &image_view) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanTextureView: failed to create image view");
        }
      }

     ~VulkanTextureView() override {
        vkDestroyImageView(device, image_view, nullptr);
      }

      void* Handle() override {
        return (void*)image_view;
      }

      Type GetType() const override {
        return type;
      }

      Texture::Format GetFormat() const override {
        return format;
      }

      u32 GetWidth() const override {
        return width;
      }

      u32 GetHeight() const override {
        return height;
      }

      Texture::Aspect GetAspect() const override {
        return range.aspect;
      }

      u32 GetBaseMip() const override {
        return range.base_mip;
      }

      u32 GetMipCount() const override {
        return range.mip_count;
      }

      u32 GetBaseLayer() const override {
        return range.base_layer;
      }

      u32 GetLayerCount() const override {
        return range.layer_count;
      }

      const Texture::SubresourceRange& GetSubresourceRange() const override {
        return range;
      }

      const ComponentMapping& GetComponentMapping() const override {
        return mapping;
      }

    private:
      VkDevice device;
      VkImageView image_view;
      Type type;
      Texture::Format format;
      u32 width;
      u32 height;
      Texture::SubresourceRange range;
      ComponentMapping mapping;
  };

} // namespace zephyr
