#include <chrono>
#include <algorithm>
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/renderer/vulkan/vulkan_instance.hpp>
#include <zephyr/renderer/render_engine.hpp>
#include <zephyr/scene/scene_graph.hpp>
#include <zephyr/scene/scene_node.hpp>
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
    void UpdateFramesPerSecondCounter();
    void CreateScene();
    void CreateBenchmarkScene();

    void CreateVulkanEngine();
    void CleanupVulkan();

    void CreateOpenGLEngine();
    void CleanupOpenGL();

    std::unique_ptr<RenderEngine> m_render_engine{};
    std::shared_ptr<SceneGraph> m_scene_graph{};
    std::shared_ptr<SceneNode> m_camera_node{};
    std::shared_ptr<SceneNode> m_behemoth_scene{};
    std::vector<SceneNode*> m_dynamic_cubes{};

    int m_fps_counter{};
    std::chrono::steady_clock::time_point m_time_point_last_update{};
    u64 m_frame{};
    SDL_Window* m_window{};
    std::shared_ptr<VulkanInstance> m_vk_instance{};

    VkSurfaceKHR m_vk_surface{VK_NULL_HANDLE};
};

} // namespace zephyr