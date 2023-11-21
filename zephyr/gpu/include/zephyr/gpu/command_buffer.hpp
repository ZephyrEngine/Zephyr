
#pragma once

#include <zephyr/gpu/bind_group.hpp>
#include <zephyr/gpu/buffer.hpp>
#include <zephyr/gpu/compute_pipeline.hpp>
#include <zephyr/gpu/enums.hpp>
#include <zephyr/gpu/pipeline_builder.hpp>
#include <zephyr/gpu/pipeline_layout.hpp>
#include <zephyr/gpu/render_target.hpp>
#include <zephyr/gpu/render_pass.hpp>
#include <zephyr/gpu/texture.hpp>
#include <zephyr/integer.hpp>
#include <span>

namespace zephyr {

  class CommandBuffer {
    public:
      enum class OneTimeSubmit {
        No = 0,
        Yes = 1
      };

      virtual ~CommandBuffer() = default;

      virtual void* Handle() = 0;

      virtual void Begin(OneTimeSubmit one_time_submit) = 0;
      virtual void End() = 0;

      virtual void SetViewport(int x, int y, int width, int height) = 0;
      virtual void SetScissor(int x, int y, int width, int height) = 0;

      virtual void CopyBuffer(
        Buffer* src,
        Buffer* dst,
        u64 size,
        u64 src_offset = 0,
        u64 dst_offset = 0
      ) = 0;

      virtual void CopyBufferToTexture(
        Buffer* buffer,
        u32 buffer_offset,
        Texture* texture,
        Texture::Layout texture_layout,
        u32 texture_mip_level
      ) = 0;

      virtual void BlitTexture2D(
        Texture* src_texture,
        Texture* dst_texture,
        const Rect2D& src_rect,
        const Rect2D& dst_rect,
        Texture::Layout src_layout,
        Texture::Layout dst_layout,
        u32 src_mip_level,
        u32 dst_mip_level,
        Sampler::FilterMode filter
      ) = 0;

      virtual void PushConstants(
        PipelineLayout* pipeline_layout,
        u32 offset,
        u32 size,
        const void* data
      ) = 0;

      virtual void BeginRenderPass(
        RenderTarget* render_target,
        RenderPass* render_pass
      ) = 0;
      virtual void EndRenderPass() = 0;

      virtual void BindPipeline(GraphicsPipeline* pipeline) = 0;
      virtual void BindPipeline(ComputePipeline* pipeline) = 0;

      virtual void BindBindGroup(
        PipelineBindPoint pipeline_bind_point,
        PipelineLayout* pipeline_layout,
        u32 set,
        BindGroup* bind_group
      ) = 0;

      virtual void BindVertexBuffers(
        std::span<Buffer* const> buffers,
        u32 first_binding = 0
      ) = 0;

      virtual void BindIndexBuffer(
        Buffer* buffer,
        IndexDataType data_type,
        size_t offset = 0
      ) = 0;

      virtual void Draw(
        u32 vertex_count,
        u32 instance_count = 1,
        u32 first_vertex = 0,
        u32 first_instance = 0
      ) = 0;

      virtual void DrawIndexed(
        u32 index_count,
        u32 instance_count = 1,
        u32 first_index = 0,
        s32 vertex_offset = 0,
        u32 first_instance = 0
      ) = 0;

      virtual void DispatchCompute(u32 group_count_x, u32 group_count_y = 1, u32 group_count_z = 1) = 0;

      virtual void Barrier(
        Texture* texture,
        PipelineStage src_stage,
        PipelineStage dst_stage,
        Access src_access,
        Access dst_access,
        Texture::Layout src_layout,
        Texture::Layout dst_layout,
        u32 mip_level,
        u32 mip_count
      ) = 0;
  };

} // namespace zephyr
