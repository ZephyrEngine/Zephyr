
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/gpu/texture.hpp>
#include <zephyr/panic.hpp>

namespace zephyr {

struct VulkanTextureView final : Texture::View {
  VulkanTextureView(
    VkDevice device,
    Texture* texture,
    Type type,
    Texture::Format format,
    u32 width,
    u32 height,
    Texture::SubresourceRange const& range,
    ComponentMapping const& mapping
  )   : device(device), type(type), format(format), width(width), height(height), range(range), mapping(mapping), texture(texture) {
    auto info = VkImageViewCreateInfo{
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

  auto Handle() -> void* override {
    return (void*)image_view;
  }

  auto GetType() const -> Type override {
    return type;
  }

  auto GetFormat() const -> Texture::Format override {
    return format;
  }

  auto GetWidth() const -> u32 override {
    return width;
  }

  auto GetHeight() const -> u32 override {
    return height;
  }

  auto GetAspect() const -> Texture::Aspect override {
    return range.aspect;
  }

  auto GetBaseMip() const -> u32 override {
    return range.base_mip;
  }

  auto GetMipCount() const -> u32 override {
    return range.mip_count;
  }

  auto GetBaseLayer() const -> u32 override {
    return range.base_layer;
  }

  auto GetLayerCount() const -> u32 override {
    return range.layer_count;
  }

  auto GetSubresourceRange() const -> Texture::SubresourceRange const& override {
    return range;
  }

  auto GetComponentMapping() const -> ComponentMapping const& override {
    return mapping;
  }

  Texture* GetTexture() override {
    return texture;
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
  Texture* texture;
};

} // namespace zephyr
