
// Copyright (C) 2022 fleroviux. All rights reserved.

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

struct CommandBuffer {
  enum class OneTimeSubmit {
    No = 0,
    Yes = 1
  };

  virtual ~CommandBuffer() = default;

  virtual auto Handle() -> void* = 0;

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
    u32 texture_mip_level = 0u
  ) = 0;

  virtual void PushConstants(
    PipelineLayout* pipeline_layout,
    u32 offset,
    u32 size,
    void const* data
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
    u32 mip_level = 0u
  ) = 0;
};

} // namespace zephyr
