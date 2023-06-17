
#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <optional>
#include <SDL.h>
#include <memory>
#include <vector>

#include "vulkan_swap_chain.hpp"
#include "window.hpp"

namespace zephyr {

  class VulkanWindow final : public WindowImpl {
    public:
      VulkanWindow();

      void SetWindowTitle(std::string_view window_title) override;
      auto GetSize() -> Size override;
      void SetSize(int width, int height) override;
      bool PollEvents() override;
      std::shared_ptr<RenderDevice>& GetRenderDevice() override { return render_device; }
      std::shared_ptr<SwapChain>& GetSwapChain() override { return swap_chain; }

      void SetOnResizeCallback(std::function<void(int, int)> callback) override;

    private:
      void CreateSDL2Window();
      void CreateVulkanContext();
      void CreateVulkanInstance();
      void CreateLogicalDevice();
      void CreateSurface();
      void CreateRenderDevice();
      void CreateSwapChain();

      std::vector<char const*> GetInstanceExtensions();
      std::vector<char const*> GetInstanceLayers();
      std::vector<char const*> GetDeviceLayers();

      VkPhysicalDevice PickPhysicalDevice();

      void HandleWindowSizeChanged();

      SDL_Window* window;
      VkInstance vulkan_instance;
      VkPhysicalDevice vulkan_physical_device;
      VkDevice vulkan_device;
      VkSurfaceKHR vulkan_surface;
      std::optional<u32> queue_family_graphics;
      std::optional<u32> queue_family_compute;

      std::shared_ptr<RenderDevice> render_device;
      std::shared_ptr<SwapChain> swap_chain;

      std::function<void(int, int)> on_resize_cb = [](int, int){};

      bool did_fire_on_resize_atleast_once = false;
  };

} // namespace zephyr