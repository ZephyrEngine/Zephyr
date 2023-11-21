
#pragma once

#include <zephyr/gpu/buffer.hpp>
#include <zephyr/gpu/enums.hpp>
#include <zephyr/gpu/sampler.hpp>
#include <zephyr/gpu/texture.hpp>
#include <zephyr/integer.hpp>
#include <memory>

namespace zephyr {

  struct BindGroup;

  class BindGroupLayout {
    public:
      struct Entry {
        u32 binding;
        BindingType type;
        ShaderStage stages = ShaderStage::All;
      };

      virtual ~BindGroupLayout() = default;

      virtual void* Handle() = 0;
  };

  class BindGroup {
    public:
      virtual ~BindGroup() = default;

      virtual void* Handle() = 0;

      virtual void Bind(u32 binding, Buffer* buffer, BindingType type) = 0;

      virtual void Bind(
        u32 binding,
        Texture::View* texture_view,
        Sampler* sampler,
        Texture::Layout layout
      ) = 0;

      void Bind(
        u32 binding,
        Texture* texture,
        Sampler* sampler,
        Texture::Layout layout
      ) {
        Bind(binding, texture->DefaultView(), sampler, layout);
      }

      virtual void Bind(
        u32 binding,
        Texture::View* texture_view,
        Texture::Layout layout,
        BindingType type
      ) = 0;

      void Bind(
        u32 binding,
        Texture* texture,
        Texture::Layout layout,
        BindingType type
      ) {
        Bind(binding, texture->DefaultView(), layout, type);
      }
  };

} // namespace zephyr
