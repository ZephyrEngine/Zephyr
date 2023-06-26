
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <array>
#include <zephyr/gpu/bind_group.hpp>
#include <zephyr/gpu/buffer.hpp>
#include <zephyr/gpu/command_buffer.hpp>
#include <zephyr/gpu/command_pool.hpp>
#include <zephyr/gpu/compute_pipeline.hpp>
#include <zephyr/gpu/fence.hpp>
#include <zephyr/gpu/pipeline_builder.hpp>
#include <zephyr/gpu/pipeline_layout.hpp>
#include <zephyr/gpu/queue.hpp>
#include <zephyr/gpu/render_target.hpp>
#include <zephyr/gpu/sampler.hpp>
#include <zephyr/gpu/shader_module.hpp>
#include <zephyr/gpu/texture.hpp>
#include <zephyr/integer.hpp>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

namespace zephyr {

struct RenderDevice {
  virtual ~RenderDevice() = default;

  virtual auto Handle() -> void* = 0;

  virtual auto CreateBuffer(
    Buffer::Usage usage,
    Buffer::Flags flags,
    size_t size
  ) -> std::unique_ptr<Buffer> = 0;

  virtual auto CreateShaderModule(
    u32 const* spirv,
    size_t size
  ) -> std::shared_ptr<ShaderModule> = 0;

  virtual auto CreateTexture2D(
    u32 width,
    u32 height,
    Texture::Format format,
    Texture::Usage usage,
    u32 mip_count = 1
  ) -> std::unique_ptr<Texture> = 0;

  virtual auto CreateTexture2DFromSwapchainImage(
    u32 width,
    u32 height,
    Texture::Format format,
    void* image_handle
  ) -> std::unique_ptr<Texture> = 0;

  virtual auto CreateTextureCube(
    u32 width,
    u32 height,
    Texture::Format format,
    Texture::Usage usage,
    u32 mip_count = 1
  ) -> std::unique_ptr<Texture> = 0;

  virtual auto CreateSampler(
    Sampler::Config const& config
  ) -> std::unique_ptr<Sampler> = 0;

  virtual auto DefaultNearestSampler() -> Sampler* = 0;
  virtual auto DefaultLinearSampler() -> Sampler* = 0;

  virtual auto CreateRenderTarget(
    std::span<Texture::View* const> color_attachments,
    Texture::View* depth_stencil_attachment = nullptr
  ) -> std::unique_ptr<RenderTarget> = 0;

  virtual auto CreateRenderPassBuilder() -> std::unique_ptr<RenderPassBuilder> = 0;

  virtual auto CreateBindGroupLayout(
    std::span<BindGroupLayout::Entry const> entries
  ) -> std::shared_ptr<BindGroupLayout> = 0;

  virtual std::unique_ptr<BindGroup> CreateBindGroup(std::shared_ptr<BindGroupLayout> layout) = 0;

  virtual auto CreatePipelineLayout(
    std::span<BindGroupLayout* const> bind_group_layouts
  ) -> std::shared_ptr<PipelineLayout> = 0;

  virtual auto CreateGraphicsPipelineBuilder() -> std::unique_ptr<GraphicsPipelineBuilder> = 0;

  virtual auto CreateComputePipeline(
    ShaderModule* shader_module,
    std::shared_ptr<PipelineLayout> layout
  ) -> std::unique_ptr<ComputePipeline> = 0;

  virtual auto CreateGraphicsCommandPool(CommandPool::Usage usage) -> std::unique_ptr<CommandPool> = 0;

  virtual auto CreateComputeCommandPool(CommandPool::Usage usage) -> std::unique_ptr<CommandPool> = 0;

  virtual auto CreateCommandBuffer(CommandPool* pool) -> std::unique_ptr<CommandBuffer> = 0;

  virtual auto CreateFence(Fence::CreateSignalled create_signalled) -> std::unique_ptr<Fence> = 0;

  virtual auto GraphicsQueue() -> Queue* = 0;

  virtual auto ComputeQueue() -> Queue* = 0;

  virtual void WaitIdle() = 0;
};

} // namespace zephyr
