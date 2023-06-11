
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <vector>

namespace zephyr {

struct VulkanRenderPass final : RenderPass {
  static constexpr size_t kMaxColorAttachments = 32;

  VulkanRenderPass(
    VkDevice device,
    VkRenderPass render_pass,
    size_t color_attachment_count,
    bool have_depth_stencil_attachment
  )   : device_(device)
      , render_pass_(render_pass)
      , color_attachment_count_(color_attachment_count)
      , has_depth_stencil_attachment(have_depth_stencil_attachment) {
    clear_values_.resize(color_attachment_count_ + (have_depth_stencil_attachment ? 1 : 0));

    InitializeClearValues();
  }

 ~VulkanRenderPass() {
    vkDestroyRenderPass(device_, render_pass_, nullptr);
  }

  auto Handle() -> VkRenderPass { return render_pass_; }

  auto GetNumberOfColorAttachments() -> size_t override {
    return color_attachment_count_;
  }

  bool HasDepthStencilAttachment() override {
    return has_depth_stencil_attachment;
  }

  auto GetClearValues() -> std::vector<VkClearValue> const& { return clear_values_; }

  void SetClearColor(int index, float r, float g, float b, float a) override {
    if (index >= color_attachment_count_) {
      ZEPHYR_PANIC("VulkanRenderPass: SetClearColor() called with an out-of-bounds index");
    }

    // @todo: handle texture formats which don't use float32
    clear_values_[index].color = VkClearColorValue{
      .float32 = {r, g, b, a}
    };
  }

  void SetClearDepth(float depth) override {
    clear_values_.back().depthStencil.depth = depth;
  }

  void SetClearStencil(u32 stencil) override {
    clear_values_.back().depthStencil.stencil = stencil;
  }

private:
  void InitializeClearValues() {
    for(int i = 0; i < GetNumberOfColorAttachments(); i++) {
      SetClearColor(i, 0.0, 0.0, 0.0, 1.0);
    }

    if(HasDepthStencilAttachment()) {
      SetClearDepth(1.0);
    }
  }

  VkDevice device_;
  VkRenderPass render_pass_;
  std::vector<VkClearValue> clear_values_;
  size_t color_attachment_count_;
  bool has_depth_stencil_attachment = false;
};

} // namespace zephyr
