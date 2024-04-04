#include <algorithm>
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/renderer2/render_engine.hpp>
#include <zephyr/scene/node.hpp>
#include <zephyr/float.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

#include "vulkan_instance.hpp"

#undef main

namespace zephyr {

  class MainWindow {
    public:
     ~MainWindow();

      void Run();

    private:
      void Setup();
      void MainLoop();
      void RenderFrame();
      void CreateVkInstance();
      VulkanPhysicalDevice* PickPhysicalDevice();
      void CreateLogicalDevice();
      void CreateSurface();
      void CreateRenderEngine();
      void CreateScene();
      void Cleanup();

      std::unique_ptr<RenderEngine> m_render_engine{};
      std::unique_ptr<SceneNode> m_scene_root{};
      u64 m_frame{};

      SDL_Window* m_window{};
      std::unique_ptr<VulkanInstance> m_vk_instance{};
      VulkanPhysicalDevice* m_vk_physical_device{};
      VkDevice m_vk_device{};
      VkQueue m_vk_graphics_compute_queue{};
      std::vector<u32> m_present_queue_family_indices{};
      std::optional<VkQueue> m_vk_dedicated_compute_queue{};
      VkSurfaceKHR m_vk_surface{VK_NULL_HANDLE};
  };

} // namespace zephyr