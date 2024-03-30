#include <algorithm>
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/float.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

#undef main

namespace zephyr {

  class MainWindow {
    public:
      void Run() {
        Setup();
        MainLoop();
        Cleanup();
      }

    private:
      void Setup();
      void MainLoop();
      void RenderFrame();
      void CreateVkInstance();
      VkPhysicalDevice PickPhysicalDevice();
      void CreateLogicalDevice();
      void CreateSurface();
      void CreateSwapChain();
      void CreateCommandPool();
      void CreateCommandBuffer();
      void CreateSemaphore();
      void CreateFence();
      void CreateRenderPass();
      void CreateFramebuffers();
      void CreateGraphicsPipeline();
      void Cleanup();

      SDL_Window* m_window{};
      VkInstance m_vk_instance{};
      VkPhysicalDevice m_vk_physical_device{};
      VkDevice m_vk_device{};
      VkQueue m_vk_graphics_compute_queue{};
      std::vector<u32> m_present_queue_family_indices{};
      std::optional<VkQueue> m_vk_dedicated_compute_queue{};
      VkSurfaceKHR m_vk_surface{VK_NULL_HANDLE};
      VkSwapchainKHR m_vk_swap_chain{};
      std::vector<VkImage> m_vk_swap_chain_images{};
      std::vector<VkImageView> m_vk_swap_chain_views{};
      VkCommandPool m_vk_command_pool{};
      VkCommandBuffer m_vk_command_buffer{};
      VkSemaphore m_vk_semaphore{};
      VkFence m_vk_fence{};
      VkRenderPass m_vk_render_pass{};
      std::vector<VkFramebuffer> m_vk_swap_chain_fbs{};
      VkPipelineLayout m_vk_pipeline_layout{};
      VkPipeline m_vk_pipeline{};
  };

} // namespace zephyr