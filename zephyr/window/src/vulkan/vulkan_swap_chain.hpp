
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/window/swap_chain.hpp>
#include <optional>
#include <vulkan/vulkan.h>
#include <vector>

namespace zephyr {

  class VulkanSwapChain final : public SwapChain {
    public:
      VulkanSwapChain(
        std::shared_ptr<RenderDevice> render_device,
        VkSurfaceKHR surface,
        int width,
        int height
      );

      std::shared_ptr<RenderTarget>& AcquireNextRenderTarget() override;
      [[nodiscard]] size_t GetNumberOfSwapChainImages() const override;
      void Present() override;
      void SetSize(int width, int height);

    private:
      void Create();
      void CreateSwapChain();
      void CreateRenderTargets();

      std::shared_ptr<RenderDevice> render_device;
      VkSurfaceKHR surface;
      VkSwapchainKHR swap_chain = VK_NULL_HANDLE;
      std::vector<std::unique_ptr<Texture>> color_textures;
      std::vector<std::unique_ptr<Texture>> depth_textures;
      std::vector<std::shared_ptr<RenderTarget>> render_targets;
      std::optional<u32> current_image_id;
      std::unique_ptr<Fence> fence;

      int width{};
      int height{};
  };

} // namespace zephyr