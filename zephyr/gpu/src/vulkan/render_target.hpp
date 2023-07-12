
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

#include "render_pass_builder.hpp"

namespace zephyr {

struct VulkanRenderTarget final : RenderTarget {
  VulkanRenderTarget(
    VkDevice device,
    std::span<Texture::View* const> color_attachments,
    Texture::View* depth_stencil_attachment
  )   : device(device)
      , color_attachments(color_attachments.begin(), color_attachments.end())
      , depth_stencil_attachment(depth_stencil_attachment) {
    auto image_views = std::vector<VkImageView>{};

    Texture::View* first_attachment = nullptr;

    if (color_attachments.empty()) {
      first_attachment = color_attachments[0];
    } else {
      if (depth_stencil_attachment == nullptr) {
        ZEPHYR_PANIC("VulkanRenderTarget: cannot have render target with zero attachments");
      }

      first_attachment = depth_stencil_attachment;
    }

    width = first_attachment->GetWidth();
    height = first_attachment->GetHeight();

    render_pass = CreateRenderPass();

    for (auto color_attachment : color_attachments) {
      image_views.push_back((VkImageView)color_attachment->Handle());
    }

    if (depth_stencil_attachment) {
      image_views.push_back((VkImageView)depth_stencil_attachment->Handle());
    }

    auto info = VkFramebufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = ((VulkanRenderPass*)render_pass.get())->Handle(),
      .attachmentCount = (u32)image_views.size(),
      .pAttachments = image_views.data(),
      .width = width,
      .height = height,
      .layers = 1
    };

    if (vkCreateFramebuffer(device, &info, nullptr, &framebuffer) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanRenderTarget: failed to create framebuffer");
    }
  }

 ~VulkanRenderTarget() override {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  auto Handle() -> void* override { return (void*)framebuffer; }
  auto GetWidth() -> u32 override { return width; }
  auto GetHeight() -> u32 override { return height; }

  std::span<Texture::View* const> GetColorAttachments() override {
    return color_attachments;
  }

private:
  auto CreateRenderPass() -> std::unique_ptr<RenderPass> {
    auto color_attachment_count = color_attachments.size();
    auto render_pass_builder = VulkanRenderPassBuilder{device};

    for (size_t i = 0; i < color_attachment_count; i++) {
      render_pass_builder.SetColorAttachmentFormat(i, color_attachments[i]->GetFormat());
      render_pass_builder.SetColorAttachmentSrcLayout(i, Texture::Layout::Undefined, std::nullopt);
      render_pass_builder.SetColorAttachmentDstLayout(i, Texture::Layout::ColorAttachment, std::nullopt);
    }

    if (depth_stencil_attachment) {
      render_pass_builder.SetDepthAttachmentFormat(depth_stencil_attachment->GetFormat());
      render_pass_builder.SetDepthAttachmentSrcLayout(Texture::Layout::Undefined, std::nullopt);
      render_pass_builder.SetDepthAttachmentDstLayout(Texture::Layout::DepthStencilAttachment, std::nullopt);
    }

    return render_pass_builder.Build();
  }

  VkDevice device;
  VkFramebuffer framebuffer;
  std::unique_ptr<RenderPass> render_pass;
  std::vector<Texture::View*> color_attachments;
  Texture::View* depth_stencil_attachment;
  u32 width;
  u32 height;
};

} // namespace zephyr
