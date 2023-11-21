
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>
#include <vk_mem_alloc.h>

#include "texture_view.hpp"

namespace zephyr {

  class VulkanTexture final : public Texture {
    public:
     ~VulkanTexture() override {
        if (image_owned) {
          vmaDestroyImage(allocator, image, allocation);
        }
      }

      void* Handle() override {
        return (void*)image;
      }

      Grade GetGrade() const override {
        return grade;
      }

      Format GetFormat() const override {
        return format;
      }

      Usage GetUsage() const override {
        return usage;
      }

      u32 GetWidth() const override {
        return width;
      }

      u32 GetHeight() const override {
        return height;
      }

      u32 GetDepth() const override {
        return depth;
      }

      u32 GetLayerCount() const override {
        return range.layer_count;
      }

      u32 GetMipCount() const override {
        return range.mip_count;
      }

      const SubresourceRange& DefaultSubresourceRange() const override {
        return range;
      }

      const View* DefaultView() const override {
        return default_view.get();
      }

      View* DefaultView() override {
        return default_view.get();
      }

      std::unique_ptr<View> CreateView(
        View::Type type,
        Format format,
        const SubresourceRange& range,
        const ComponentMapping& mapping = {}
      ) override {
        return std::make_unique<VulkanTextureView>(device, this, type, format, width, height, range, mapping);
      }

      static std::unique_ptr<VulkanTexture> Create2D(
        VkDevice device,
        VmaAllocator allocator,
        u32 width,
        u32 height,
        u32 mip_count,
        Format format,
        Usage usage
      ) {
        return Create(device, allocator, format, usage, Grade::_2D, View::Type::_2D, width, height, 1, mip_count);
      }

      static std::unique_ptr<VulkanTexture> CreateCube(
        VkDevice device,
        VmaAllocator allocator,
        u32 width,
        u32 height,
        u32 mip_count,
        Format format,
        Usage usage
      ) {
        return Create(device, allocator, format, usage, Grade::_2D, View::Type::Cube, width, height, 1, mip_count, 6);
      }

      static std::unique_ptr<VulkanTexture> Create2DFromSwapchain(
        VkDevice device,
        uint width,
        uint height,
        Format format,
        VkImage image
      ) {
        auto texture = std::make_unique<VulkanTexture>();

        texture->device = device;
        texture->image = image;
        texture->grade = Grade::_2D;
        texture->format = format;
        texture->usage = Usage::ColorAttachment;
        texture->width = width;
        texture->height = height;
        texture->depth = 1;
        texture->range = { (Aspect)GetAspectBits(format), 0, 1, 0, 1 };
        texture->image_owned = false;
        texture->default_view = texture->CreateView(
          View::Type::_2D, format, texture->DefaultSubresourceRange());

        return texture;
      }

    private:
      static std::unique_ptr<VulkanTexture> Create(
        VkDevice device,
        VmaAllocator allocator,
        Format format,
        Usage usage,
        Grade grade,
        View::Type default_view_type,
        u32 width,
        u32 height,
        u32 depth = 1,
        u32 mip_count = 1,
        u32 layer_count = 1,
        VkImageCreateFlags flags = 0
      ) {
        auto texture = std::make_unique<VulkanTexture>();

        if (default_view_type == View::Type::Cube || default_view_type == View::Type::CubeArray) {
          flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        auto image_info = VkImageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = flags,
          .imageType = (VkImageType)grade,
          .format = (VkFormat)format,
          .extent = VkExtent3D{
            .width = width,
            .height = height,
            .depth = depth
          },
          .mipLevels = mip_count,
          .arrayLayers = layer_count,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .tiling = VK_IMAGE_TILING_OPTIMAL,
          .usage = (VkImageUsageFlags)usage,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .queueFamilyIndexCount = 0,
          .pQueueFamilyIndices = nullptr,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        auto alloc_info = VmaAllocationCreateInfo{
          .usage = VMA_MEMORY_USAGE_AUTO
        };

        if (vmaCreateImage(allocator, &image_info, &alloc_info, &texture->image, &texture->allocation, nullptr) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanTexture: failed to create image");
        }

        texture->device = device;
        texture->allocator = allocator;
        texture->grade = grade;
        texture->format = format;
        texture->usage = usage;
        texture->width = width;
        texture->height = height;
        texture->depth = depth;
        texture->range = { (Aspect)GetAspectBits(format), 0, mip_count, 0, layer_count };
        texture->image_owned = true;
        texture->default_view = texture->CreateView(
          default_view_type, format, texture->DefaultSubresourceRange());

        return texture;
      }

      static VkImageAspectFlags GetAspectBits(Format format) {
        switch (format) {
          case Format::R8G8B8A8_UNORM:
          case Format::R8G8B8A8_SRGB:
          case Format::B8G8R8A8_SRGB:
          case Format::R32G32B32A32_SFLOAT:
            return VK_IMAGE_ASPECT_COLOR_BIT;
          case Format::DEPTH_U16:
          case Format::DEPTH_F32:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
          default:
            ZEPHYR_PANIC("VulkanTexture: cannot get image aspect for format: 0x{:08X}", (u32)format);
        }
      }

      VkDevice device;
      VmaAllocator allocator;
      VmaAllocation allocation;
      VkImage image;
      Grade grade;
      Format format;
      Usage usage;
      u32 width;
      u32 height;
      u32 depth;
      SubresourceRange range;
      std::unique_ptr<View> default_view;
      bool image_owned;
  };

} // namespace zephyr
