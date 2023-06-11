
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/integer.hpp>
#include <zephyr/gpu/texture.hpp>
#include <optional>

namespace zephyr {

struct RenderPass {
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkAttachmentLoadOp
  enum class LoadOp {
    Load = 0,
    Clear = 1,
    DontCare = 2
  };

  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkAttachmentStoreOp
  enum class StoreOp {
    Store = 0,
    DontCare = 1
  };

  virtual ~RenderPass() = default;

  virtual auto GetNumberOfColorAttachments() -> size_t = 0;
  virtual bool HasDepthStencilAttachment() = 0;

  virtual void SetClearColor(int index, float r, float g, float b, float a) = 0;
  virtual void SetClearDepth(float depth) = 0;
  virtual void SetClearStencil(u32 stencil) = 0;
};

struct RenderPassBuilder {
  struct Transition {
    PipelineStage stages;
    Access access;
  };

  virtual ~RenderPassBuilder() = default;

  virtual void SetColorAttachmentCount(size_t count) = 0;
  virtual void SetColorAttachmentFormat(size_t id, Texture::Format format) = 0;
  virtual void SetColorAttachmentSrcLayout(size_t id, Texture::Layout layout, std::optional<Transition> transition) = 0;
  virtual void SetColorAttachmentDstLayout(size_t id, Texture::Layout layout, std::optional<Transition> transition) = 0;
  virtual void SetColorAttachmentLoadOp(size_t id, RenderPass::LoadOp op) = 0;
  virtual void SetColorAttachmentStoreOp(size_t id, RenderPass::StoreOp op) = 0;

  virtual void ClearDepthAttachment() = 0;
  virtual void SetDepthAttachmentFormat(Texture::Format format) = 0;
  virtual void SetDepthAttachmentSrcLayout(Texture::Layout layout, std::optional<Transition> transition) = 0;
  virtual void SetDepthAttachmentDstLayout(Texture::Layout layout, std::optional<Transition> transition) = 0;
  virtual void SetDepthAttachmentLoadOp(RenderPass::LoadOp op) = 0;
  virtual void SetDepthAttachmentStoreOp(RenderPass::StoreOp op) = 0;
  virtual void SetDepthAttachmentStencilLoadOp(RenderPass::LoadOp op) = 0;
  virtual void SetDepthAttachmentStencilStoreOp(RenderPass::StoreOp op) = 0;

  virtual auto Build() const -> std::unique_ptr<RenderPass> = 0;
};

} // namespace zephyr
