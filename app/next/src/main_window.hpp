#include <algorithm>
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/renderer/vulkan/vulkan_instance.hpp>
#include <zephyr/renderer/render_engine.hpp>
#include <zephyr/scene/node.hpp>
#include <zephyr/float.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

#undef main

#define ZEPHYR_OPENGL

namespace zephyr {

  class MainWindow {
    public:
     ~MainWindow();

      void Run();

    private:
      void Setup();
      void MainLoop();
      void RenderFrame();
      void CreateScene();

      void CreateVulkanEngine();
      void CleanupVulkan();

      void CreateOpenGLEngine();
      void CleanupOpenGL();

      std::unique_ptr<RenderEngine> m_render_engine{};
      std::unique_ptr<SceneNode> m_scene_root{};
      u64 m_frame{};

      SDL_Window* m_window{};
      std::shared_ptr<VulkanInstance> m_vk_instance{};
      VkSurfaceKHR m_vk_surface{VK_NULL_HANDLE};

      SceneNode* m_behemoth_scene{};
  };

} // namespace zephyr