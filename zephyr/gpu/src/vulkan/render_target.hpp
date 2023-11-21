
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>

#include "render_pass_builder.hpp"

namespace zephyr {

  class VulkanRenderTarget final : public RenderTarget {
    public:
      VulkanRenderTarget(
        VkDevice device,
        std::span<Texture::View* const> color_attachments,
        Texture::View* depth_stencil_attachment
      )   : m_device{device}
          , m_color_attachments{color_attachments.begin(), color_attachments.end()}
          , m_depth_stencil_attachment{depth_stencil_attachment} {
        std::vector<VkImageView> image_views{};

        Texture::View* first_attachment = nullptr;

        if(color_attachments.empty()) {
          first_attachment = color_attachments[0];
        } else {
          if(depth_stencil_attachment == nullptr) {
            ZEPHYR_PANIC("VulkanRenderTarget: cannot have render target with zero attachments");
          }

          first_attachment = depth_stencil_attachment;
        }

        m_width = first_attachment->GetWidth();
        m_height = first_attachment->GetHeight();
        m_render_pass = CreateRenderPass();

        for(auto color_attachment : color_attachments) {
          image_views.push_back((VkImageView)color_attachment->Handle());
        }

        if(depth_stencil_attachment) {
          image_views.push_back((VkImageView)depth_stencil_attachment->Handle());
        }

        const VkFramebufferCreateInfo info{
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .renderPass = ((VulkanRenderPass*)m_render_pass.get())->Handle(),
          .attachmentCount = (u32)image_views.size(),
          .pAttachments = image_views.data(),
          .width = m_width,
          .height = m_height,
          .layers = 1
        };

        if(vkCreateFramebuffer(device, &info, nullptr, &m_framebuffer) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanRenderTarget: failed to create framebuffer");
        }
      }

     ~VulkanRenderTarget() override {
        vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
      }

      void* Handle() override {
        return (void*)m_framebuffer;
      }

      u32 GetWidth() override {
        return m_width;
      }

      u32 GetHeight() override {
        return m_height;
      }

    private:
      std::unique_ptr<RenderPass> CreateRenderPass() {
        auto color_attachment_count = m_color_attachments.size();
        auto render_pass_builder = VulkanRenderPassBuilder{m_device};

        for(size_t i = 0; i < color_attachment_count; i++) {
          render_pass_builder.SetColorAttachmentFormat(i, m_color_attachments[i]->GetFormat());
          render_pass_builder.SetColorAttachmentSrcLayout(i, Texture::Layout::Undefined, std::nullopt);
          render_pass_builder.SetColorAttachmentDstLayout(i, Texture::Layout::ColorAttachment, std::nullopt);
        }

        if(m_depth_stencil_attachment) {
          render_pass_builder.SetDepthAttachmentFormat(m_depth_stencil_attachment->GetFormat());
          render_pass_builder.SetDepthAttachmentSrcLayout(Texture::Layout::Undefined, std::nullopt);
          render_pass_builder.SetDepthAttachmentDstLayout(Texture::Layout::DepthStencilAttachment, std::nullopt);
        }

        return render_pass_builder.Build();
      }

      VkDevice m_device;
      VkFramebuffer m_framebuffer;
      std::unique_ptr<RenderPass> m_render_pass;
      std::vector<Texture::View*> m_color_attachments;
      Texture::View* m_depth_stencil_attachment;
      u32 m_width;
      u32 m_height;
};

} // namespace zephyr
