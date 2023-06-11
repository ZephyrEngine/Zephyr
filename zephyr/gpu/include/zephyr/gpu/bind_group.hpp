
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/buffer.hpp>
#include <zephyr/gpu/sampler.hpp>
#include <zephyr/gpu/texture.hpp>
#include <zephyr/integer.hpp>
#include <memory>

namespace zephyr {

struct BindGroup;

struct BindGroupLayout {
  struct Entry {
    // subset of VkDescriptorType:
    // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkDescriptorType
    enum class Type : u32 {
      Sampler = 0,
      ImageWithSampler = 1,
      SampledImage = 2,
      StorageImage = 3,
      UniformBuffer = 6
    };

    // subset of VkShaderStageFlagBits:
    // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkShaderStageFlagBits
    enum class ShaderStage : u32 {
      Vertex = 0x00000001,
      Fragment = 0x00000010,
      All = 0x7FFFFFFF
    };

    u32 binding;
    Type type;
    ShaderStage stages = ShaderStage::All;
  };

  virtual ~BindGroupLayout() = default;

  virtual auto Handle() -> void* = 0;
  virtual auto Instantiate() -> std::unique_ptr<BindGroup> = 0;
};

constexpr auto operator|(
  BindGroupLayout::Entry::ShaderStage lhs,
  BindGroupLayout::Entry::ShaderStage rhs
) -> BindGroupLayout::Entry::ShaderStage {
  return (BindGroupLayout::Entry::ShaderStage)((u32)lhs | (u32)rhs);
}

struct BindGroup {
  virtual ~BindGroup() = default;

  virtual auto Handle() -> void* = 0;

  virtual void Bind(
    u32 binding,
    Buffer* buffer,
    BindGroupLayout::Entry::Type type
  ) = 0;

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
    BindGroupLayout::Entry::Type type
  ) = 0;

  void Bind(
    u32 binding,
    Texture* texture,
    Texture::Layout layout,
    BindGroupLayout::Entry::Type type
  ) {
    Bind(binding, texture->DefaultView(), layout, type);
  }
};

} // namespace zephyr
