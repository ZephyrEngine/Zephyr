
#pragma once

#include <zephyr/integer.hpp>
#include <memory>

namespace zephyr {

  // equivalent to VkComponentSwizzle:
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkComponentSwizzle.html
  enum class ComponentSwizzle {
    Identity = 0,
    Zero = 1,
    One = 2,
    R = 3,
    G = 4,
    B = 5,
    A = 6
  };

  struct ComponentMapping {
    ComponentSwizzle r = ComponentSwizzle::Identity;
    ComponentSwizzle g = ComponentSwizzle::Identity;
    ComponentSwizzle b = ComponentSwizzle::Identity;
    ComponentSwizzle a = ComponentSwizzle::Identity;
  };

  class Texture {
    public:
      // subset of VkImageLayout:
      // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkImageLayout
      enum class Layout {
        Undefined = 0,
        General = 1,
        ColorAttachment = 2,
        DepthStencilAttachment = 3,
        DepthStencilReadOnly = 4,
        ShaderReadOnly = 5,
        CopySrc = 6,
        CopyDst = 7,

        // VK_KHR_swapchain
        PresentSrc = 1000001002,

        // Vulkan 1.2
        DepthAttachment = 1000241000,
        DepthReadOnly = 1000241001,
        StencilAttachment = 1000241002,
        StencilReadOnly = 1000241003,
      };

      // equivalent to VkImageType:
      // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkImageType.html
      enum class Grade {
        _1D = 0,
        _2D = 1,
        _3D = 2
      };

      // subset of VkFormat:
      // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkFormat
      // @todo: find a subset of formats that work on all targeted platforms.
      enum class Format {
        R8G8B8A8_UNORM = 37,
        R8G8B8A8_SRGB = 43,
        B8G8R8A8_SRGB = 50,
        R32G32B32A32_SFLOAT = 109,
        DEPTH_U16 = 124,
        DEPTH_F32 = 126
      };

      // subset of VkImageUsageFlagBits:
      // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkImageUsageFlagBits
      enum class Usage : u32 {
        CopySrc = 0x00000001,
        CopyDst = 0x00000002,
        Sampled = 0x00000004,
        Storage = 0x00000008,
        ColorAttachment = 0x00000010,
        DepthStencilAttachment = 0x00000020
      };

      // subset of VkImageAspectFlags:
      // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkImageAspectFlagBits.html
      enum class Aspect : u32 {
        Color = 0x00000001,
        Depth = 0x00000002,
        Stencil = 0x00000004,
        Metadata = 0x00000008
      };

      struct SubresourceLayers {
        Aspect aspect;
        u32 mip_level;
        u32 base_layer;
        u32 layer_count;
      };

      struct SubresourceRange {
        Aspect aspect;
        u32 base_mip;
        u32 mip_count;
        u32 base_layer;
        u32 layer_count;

        SubresourceRange() = default;

        SubresourceRange(Aspect aspect, u32 base_mip, u32 mip_count, u32 base_layer, u32 layer_count)
            : aspect{aspect}, base_mip{base_mip}, mip_count{mip_count}, base_layer{base_layer}, layer_count{layer_count} {
        }

        SubresourceRange(const SubresourceLayers& layers)
            : aspect{layers.aspect}, base_mip{layers.mip_level}, mip_count{1}, base_layer{layers.base_layer}, layer_count{layers.layer_count} {
        }

        SubresourceLayers Layers(u32 mip_level) const {
          return {
            .aspect = aspect,
            .mip_level = mip_level,
            .base_layer = base_layer,
            .layer_count = layer_count
          };
        }
      };

      class View {
        public:
          // equivalent to VkImageViewType:
          // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#VkImageViewType
          enum class Type {
            _1D = 0,
            _2D = 1,
            _3D = 2,
            Cube = 3,
            _1D_Array = 4,
            _2D_Array = 5,
            CubeArray = 6
          };

          virtual ~View() = default;

          virtual void* Handle() = 0;
          virtual Type GetType() const = 0;
          virtual Texture::Format GetFormat() const = 0;
          virtual u32 GetWidth() const = 0;
          virtual u32 GetHeight() const = 0;
          virtual Texture::Aspect GetAspect() const = 0;
          virtual u32 GetBaseMip() const = 0;
          virtual u32 GetMipCount() const = 0;
          virtual u32 GetBaseLayer() const = 0;
          virtual u32 GetLayerCount() const = 0;
          virtual const Texture::SubresourceRange& GetSubresourceRange() const = 0;
          virtual const ComponentMapping& GetComponentMapping() const = 0;
      };

      virtual ~Texture() = default;

      virtual void* Handle() = 0;
      virtual Grade GetGrade() const = 0;
      virtual Format GetFormat() const = 0;
      virtual Usage GetUsage() const = 0;
      virtual u32 GetWidth() const = 0;
      virtual u32 GetHeight() const = 0;
      virtual u32 GetDepth() const = 0;
      virtual u32 GetLayerCount() const = 0;
      virtual u32 GetMipCount() const = 0;

      virtual const SubresourceRange& DefaultSubresourceRange() const = 0;

      virtual const View* DefaultView() const = 0;
      virtual View* DefaultView() = 0;

      virtual std::unique_ptr<View> CreateView(
        View::Type type,
        Format format,
        const SubresourceRange& range,
        const ComponentMapping& mapping = {}
      ) = 0;
  };

  constexpr Texture::Usage operator|(Texture::Usage lhs, Texture::Usage rhs) {
    return static_cast<Texture::Usage>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
  }

  constexpr Texture::Aspect operator|(Texture::Aspect lhs, Texture::Aspect rhs) {
    return static_cast<Texture::Aspect>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
  }

} // namespace zephyr
