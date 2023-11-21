
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <vector>

namespace zephyr {

  class VulkanRenderPass final : public RenderPass {
    public:
      static constexpr size_t k_max_color_attachments = 32;

      VulkanRenderPass(
        VkDevice device,
        VkRenderPass render_pass,
        size_t color_attachment_count,
        bool have_depth_stencil_attachment
      )   : m_device{device}
          , m_render_pass{render_pass}
          , m_color_attachment_count{color_attachment_count}
          , m_has_depth_stencil_attachment{have_depth_stencil_attachment} {
        m_clear_values.resize(m_color_attachment_count + (have_depth_stencil_attachment ? 1 : 0));

        InitializeClearValues();
      }

     ~VulkanRenderPass() {
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);
      }

      VkRenderPass Handle() {
        return m_render_pass;
      }

      auto GetNumberOfColorAttachments() const -> size_t override {
        return m_color_attachment_count;
      }

      bool HasDepthStencilAttachment() const override {
        return m_has_depth_stencil_attachment;
      }

      const std::vector<VkClearValue>& GetClearValues() {
        return m_clear_values;
      }

      void SetClearColor(int index, float r, float g, float b, float a) override {
        if(index >= m_color_attachment_count) {
          ZEPHYR_PANIC("VulkanRenderPass: SetClearColor() called with an out-of-bounds index");
        }

        // @todo: handle texture formats which don't use float32
        m_clear_values[index].color = VkClearColorValue{
          .float32 = {r, g, b, a}
        };
      }

      void SetClearDepth(float depth) override {
        m_clear_values.back().depthStencil.depth = depth;
      }

      void SetClearStencil(u32 stencil) override {
        m_clear_values.back().depthStencil.stencil = stencil;
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

      VkDevice m_device;
      VkRenderPass m_render_pass;
      std::vector<VkClearValue> m_clear_values;
      size_t m_color_attachment_count;
      bool m_has_depth_stencil_attachment{false};
  };

} // namespace zephyr
