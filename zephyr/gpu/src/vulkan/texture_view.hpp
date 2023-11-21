
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
        const Texture::SubresourceRange& range,
        const ComponentMapping& mapping
      )   : m_device{device}
          , m_type{type}
          , m_format{format}
          , m_width{width}
          , m_height{height}
          , m_range{range}
          , m_mapping{mapping} {
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

        if(vkCreateImageView(device, &info, nullptr, &m_image_view) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanTextureView: failed to create image view");
        }
      }

     ~VulkanTextureView() override {
        vkDestroyImageView(m_device, m_image_view, nullptr);
      }

      void* Handle() override {
        return (void*)m_image_view;
      }

      Type GetType() const override {
        return m_type;
      }

      Texture::Format GetFormat() const override {
        return m_format;
      }

      u32 GetWidth() const override {
        return m_width;
      }

      u32 GetHeight() const override {
        return m_height;
      }

      Texture::Aspect GetAspect() const override {
        return m_range.aspect;
      }

      u32 GetBaseMip() const override {
        return m_range.base_mip;
      }

      u32 GetMipCount() const override {
        return m_range.mip_count;
      }

      u32 GetBaseLayer() const override {
        return m_range.base_layer;
      }

      u32 GetLayerCount() const override {
        return m_range.layer_count;
      }

      const Texture::SubresourceRange& GetSubresourceRange() const override {
        return m_range;
      }

      const ComponentMapping& GetComponentMapping() const override {
        return m_mapping;
      }

    private:
      VkDevice m_device;
      VkImageView m_image_view;
      Type m_type;
      Texture::Format m_format;
      u32 m_width;
      u32 m_height;
      Texture::SubresourceRange m_range;
      ComponentMapping m_mapping;
  };

} // namespace zephyr
