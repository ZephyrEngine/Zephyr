
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

  class RenderDevice {
    public:
      virtual ~RenderDevice() = default;

      virtual void* Handle() = 0;

      virtual std::unique_ptr<Buffer> CreateBuffer(Buffer::Usage usage, Buffer::Flags flags, size_t size) = 0;

      virtual std::shared_ptr<ShaderModule> CreateShaderModule(u32 const* spirv, size_t size) = 0;

      virtual std::unique_ptr<Texture> CreateTexture2D(
        u32 width,
        u32 height,
        Texture::Format format,
        Texture::Usage usage,
        u32 mip_count = 1
      ) = 0;

      virtual std::unique_ptr<Texture> CreateTexture2DFromSwapchainImage(
        u32 width,
        u32 height,
        Texture::Format format,
        void* image_handle
      ) = 0;

      virtual std::unique_ptr<Texture> CreateTextureCube(
        u32 width,
        u32 height,
        Texture::Format format,
        Texture::Usage usage,
        u32 mip_count = 1
      ) = 0;

      virtual std::unique_ptr<Sampler> CreateSampler(const Sampler::Config& config) = 0;

      virtual Sampler* DefaultNearestSampler()  = 0;
      virtual Sampler* DefaultLinearSampler()   = 0;

      virtual std::unique_ptr<RenderTarget> CreateRenderTarget(
        std::span<Texture::View* const> color_attachments,
        Texture::View* depth_stencil_attachment = nullptr
      ) = 0;

      virtual std::unique_ptr<RenderPassBuilder> CreateRenderPassBuilder() = 0;

      virtual std::shared_ptr<BindGroupLayout> CreateBindGroupLayout(std::span<const BindGroupLayout::Entry> entries) = 0;

      virtual std::unique_ptr<BindGroup> CreateBindGroup(std::shared_ptr<BindGroupLayout> layout) = 0;

      virtual std::shared_ptr<PipelineLayout> CreatePipelineLayout(std::span<BindGroupLayout* const> bind_group_layouts) = 0;

      virtual std::unique_ptr<GraphicsPipelineBuilder> CreateGraphicsPipelineBuilder() = 0;

      virtual std::unique_ptr<ComputePipeline> CreateComputePipeline(
        ShaderModule* shader_module,
        std::shared_ptr<PipelineLayout> layout
      ) = 0;

      virtual std::shared_ptr<CommandPool> CreateGraphicsCommandPool(CommandPool::Usage usage) = 0;

      virtual std::shared_ptr<CommandPool> CreateComputeCommandPool(CommandPool::Usage usage) = 0;

      virtual std::unique_ptr<CommandBuffer> CreateCommandBuffer(std::shared_ptr<CommandPool> pool) = 0;

      virtual std::unique_ptr<Fence> CreateFence(Fence::CreateSignalled create_signalled) = 0;

      virtual Queue* GraphicsQueue() = 0;

      virtual Queue* ComputeQueue() = 0;

      virtual void WaitIdle() = 0;
  };

} // namespace zephyr
