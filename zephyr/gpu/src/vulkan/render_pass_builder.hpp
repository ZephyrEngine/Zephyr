
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <algorithm>
#include <array>
#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>
#include <zephyr/vector_n.hpp>

#include "render_pass.hpp"

namespace zephyr {

struct VulkanRenderPassBuilder final : RenderPassBuilder {
  VulkanRenderPassBuilder(VkDevice device) : device(device) {
    for(size_t i = 0; i < color_attachments.size(); i++) {
      color_attachments[i].descriptor = VkAttachmentDescription{
        .flags = 0,
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      };
    }

    depth_stencil_attachment.descriptor = VkAttachmentDescription{
      .flags = 0,
      .format = VK_FORMAT_D24_UNORM_S8_UINT,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
  }

  void SetColorAttachmentCount(size_t count) override {
    color_attachment_count = count;
  }

  void SetColorAttachmentFormat(size_t id, Texture::Format format) override {
    Expand(id);
    color_attachments.at(id).descriptor.format = (VkFormat)format;
  }

  void SetColorAttachmentSrcLayout(size_t id, Texture::Layout layout, std::optional<Transition> transition) override {
    Expand(id);
    color_attachments.at(id).descriptor.initialLayout = (VkImageLayout)layout;
    color_attachments.at(id).transitions.initial = transition;
  }

  void SetColorAttachmentDstLayout(size_t id, Texture::Layout layout, std::optional<Transition> transition) override {
    Expand(id);
    color_attachments.at(id).descriptor.finalLayout = (VkImageLayout)layout;
    color_attachments.at(id).transitions.final = transition;
  }

  void SetColorAttachmentLoadOp(size_t id, RenderPass::LoadOp op) override {
    Expand(id);
    color_attachments.at(id).descriptor.loadOp = (VkAttachmentLoadOp)op;
  }

  void SetColorAttachmentStoreOp(size_t id, RenderPass::StoreOp op) override {
    Expand(id);
    color_attachments.at(id).descriptor.storeOp = (VkAttachmentStoreOp)op;
  }

  void ClearDepthAttachment() override {
    have_depth_stencil_attachment = false;
  }

  void SetDepthAttachmentFormat(Texture::Format format) override {
    depth_stencil_attachment.descriptor.format = (VkFormat)format;
    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentSrcLayout(Texture::Layout layout, std::optional<Transition> transition) override {
    depth_stencil_attachment.descriptor.initialLayout = (VkImageLayout)layout;
    depth_stencil_attachment.transitions.initial = transition;

    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentDstLayout(Texture::Layout layout, std::optional<Transition> transition) override {
    depth_stencil_attachment.descriptor.finalLayout = (VkImageLayout)layout;
    depth_stencil_attachment.transitions.final = transition;

    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentLoadOp(RenderPass::LoadOp op) override {
    depth_stencil_attachment.descriptor.loadOp = (VkAttachmentLoadOp)op;

    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentStoreOp(RenderPass::StoreOp op) override {
    depth_stencil_attachment.descriptor.storeOp = (VkAttachmentStoreOp)op;

    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentStencilLoadOp(RenderPass::LoadOp op) override {
    depth_stencil_attachment.descriptor.stencilLoadOp = (VkAttachmentLoadOp)op;

    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentStencilStoreOp(RenderPass::StoreOp op) override {
    depth_stencil_attachment.descriptor.stencilStoreOp = (VkAttachmentStoreOp)op;
    have_depth_stencil_attachment = true;
  }

  auto Build() const -> std::unique_ptr<RenderPass> override {
    bool have_initial_color_layout_transition = false;
    bool have_initial_depth_layout_transition = false;

    bool have_final_color_layout_transition = false;
    bool have_final_depth_layout_transition = false;

    size_t attachment_count = color_attachment_count;

    if(have_depth_stencil_attachment) {
      attachment_count++;
    }

    VkAttachmentDescription descriptors[attachment_count];
    VkAttachmentReference references[attachment_count];

    auto initial_dependency_stages = PipelineStage::None;
    auto initial_dependency_access = Access::None;

    auto final_dependency_stages = PipelineStage::None;
    auto final_dependency_access = Access::None;

    for(size_t i = 0; i < color_attachment_count; i++) {
      auto const& attachment  = color_attachments[i];
      auto const& transitions = attachment.transitions;

      descriptors[i] = attachment.descriptor;
      references[i].attachment = i;
      references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      const auto initial_layout = descriptors[i].initialLayout;

      if(
        initial_layout != VK_IMAGE_LAYOUT_UNDEFINED &&
        initial_layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      ) {
        if(!transitions.initial.has_value()) {
          ZEPHYR_PANIC(
            "VulkanRenderPassBuilder: color attachment #{} initial layout requires transition information", i);
        }

        auto const& transition = transitions.initial.value();
        initial_dependency_stages = initial_dependency_stages | transition.stages;
        initial_dependency_access = initial_dependency_access | transition.access;

        have_initial_color_layout_transition = true;
      }

      const auto final_layout = descriptors[i].finalLayout;

      if(
        final_layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
        final_layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      ) {
        if(!transitions.final.has_value()) {
          ZEPHYR_PANIC(
            "VulkanRenderPassBuilder: color attachment #{} final layout requires transition information", i);
        }

        auto const& transition = transitions.final.value();
        final_dependency_stages = final_dependency_stages | transition.stages;
        final_dependency_access = final_dependency_access | transition.access;

        have_final_color_layout_transition = true;
      }
    }

    if(have_depth_stencil_attachment) {
      const auto& attachment  = depth_stencil_attachment;
      const auto& transitions = attachment.transitions;

      auto& descriptor = descriptors[color_attachment_count];
      auto& reference  = references [color_attachment_count];

      descriptor = depth_stencil_attachment.descriptor;
      reference.attachment = color_attachment_count;
      reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      const auto initial_layout = descriptor.initialLayout;

      if(
        initial_layout != VK_IMAGE_LAYOUT_UNDEFINED &&
        initial_layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
      ) {
        if(!transitions.initial.has_value()) {
          ZEPHYR_PANIC(
            "VulkanRenderPassBuilder: depth/stencil descriptor initial layout required transition information");
        }

        auto const& transition = transitions.initial.value();
        initial_dependency_stages = initial_dependency_stages | transition.stages;
        initial_dependency_access = initial_dependency_access | transition.access;

        have_initial_depth_layout_transition = true;
      }

      const auto final_layout = descriptor.finalLayout;

      if(final_layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        if(!transitions.final.has_value()) {
          ZEPHYR_PANIC(
            "VulkanRenderPassBuilder: depth/stencil descriptor final layout requires transition information");
        }

        auto const& transition = transitions.final.value();
        final_dependency_stages = final_dependency_stages | transition.stages;
        final_dependency_access = final_dependency_access | transition.access;

        have_final_depth_layout_transition = true;
      }
    }

    auto sub_pass = VkSubpassDescription{};
    sub_pass.flags = 0;
    sub_pass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub_pass.inputAttachmentCount = 0;
    sub_pass.pInputAttachments = nullptr;
    sub_pass.colorAttachmentCount = color_attachment_count;
    sub_pass.pColorAttachments = references;
    sub_pass.pResolveAttachments = nullptr;

    if(have_depth_stencil_attachment) {
      sub_pass.pDepthStencilAttachment = &references[color_attachment_count];
    } else {
      sub_pass.pDepthStencilAttachment = nullptr;
    }

    auto info = VkRenderPassCreateInfo{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.attachmentCount = attachment_count;
    info.pAttachments = descriptors;
    info.subpassCount = 1;
    info.pSubpasses = &sub_pass;

    Vector_N<VkSubpassDependency, 2> dependencies;

    if(have_initial_color_layout_transition || have_initial_depth_layout_transition) {
      auto dst_stage  = PipelineStage::ColorAttachmentOutput;
      auto dst_access = Access::ColorAttachmentRead;

      if(have_initial_depth_layout_transition) {
        dst_stage  = PipelineStage::EarlyFragmentTests;
        dst_access = Access::DepthStencilAttachmentRead;
      }

      dependencies.PushBack(VkSubpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = (VkPipelineStageFlags)initial_dependency_stages,
        .dstStageMask = (VkPipelineStageFlags)dst_stage,
        .srcAccessMask = (VkAccessFlags)initial_dependency_access,
        .dstAccessMask = (VkAccessFlags)dst_access,
        .dependencyFlags = 0
      });
    }

    if(have_final_color_layout_transition || have_final_depth_layout_transition) {
      Access src_access = Access::None;

      if(have_final_color_layout_transition) {
        src_access = src_access | Access::ColorAttachmentWrite;
      }

      if(have_final_depth_layout_transition) {
        src_access = src_access | Access::DepthStencilAttachmentWrite;
      }

      dependencies.PushBack(VkSubpassDependency{
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = (VkPipelineStageFlags)final_dependency_stages,
        .srcAccessMask = (VkAccessFlags)src_access,
        .dstAccessMask = (VkAccessFlags)final_dependency_access,
        .dependencyFlags = 0
      });
    }

    info.dependencyCount = (u32)dependencies.Size();
    info.pDependencies = dependencies.Data();

    VkRenderPass render_pass;

    if(vkCreateRenderPass(device, &info, nullptr, &render_pass) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanRenderPassBuilder: failed to create a render pass");
    }

    return std::make_unique<VulkanRenderPass>(device, render_pass, color_attachment_count, have_depth_stencil_attachment);
  }

private:
  static constexpr size_t kMaxColorAttachments = VulkanRenderPass::kMaxColorAttachments;

  struct Attachment {
    VkAttachmentDescription descriptor{};

    struct Transitions {
      std::optional<Transition> initial;
      std::optional<Transition> final;
    } transitions{};
  };

  void Expand(size_t id) {
    color_attachment_count = std::max(color_attachment_count, id + 1);
  }

  VkDevice device;

  size_t color_attachment_count = 0;
  std::array<Attachment, kMaxColorAttachments> color_attachments;

  bool have_depth_stencil_attachment = false;
  Attachment depth_stencil_attachment;
};

} // namespace zephyr
