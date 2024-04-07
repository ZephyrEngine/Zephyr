
#include <zephyr/renderer2/backend/render_backend_vk.hpp>

#include "main_window.hpp"

static const bool enable_validation_layers = true;

namespace zephyr {

  MainWindow::~MainWindow() {
    Cleanup();
  }

  void MainWindow::Run() {
    Setup();
    MainLoop();
  }

  void MainWindow::Setup() {
    m_window = SDL_CreateWindow(
      "Zephyr Next",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      1920,
      1080,
      SDL_WINDOW_VULKAN
    );

    CreateVkInstance();

    m_vk_physical_device = PickPhysicalDevice();
    if(m_vk_physical_device == nullptr) {
      ZEPHYR_PANIC("Failed to find a suitable GPU!");
    }

    CreateLogicalDevice();
    CreateSurface();
    CreateRenderEngine();
    CreateScene();
  }

  void MainWindow::MainLoop() {
    SDL_Event event{};

    while(true) {
      while(SDL_PollEvent(&event)) {
        if(event.type == SDL_QUIT) {
          return;
        }
      }

      RenderFrame();
    }
  }

  void MainWindow::RenderFrame() {
    // Update the transform of the second cube
    const float angle = (f32)m_frame * 0.01f;
    auto& cube_b = m_scene_root->GetChildren()[0]->GetChildren()[0];
    cube_b->GetTransform().GetPosition() = Vector3{std::cos(angle), 0.0f, -std::sin(angle)} * 2.0f;
    cube_b->GetTransform().GetRotation().SetFromEuler(0.0f, angle, 0.0f);

    m_scene_root->Traverse([&](SceneNode* node) {
      node->GetTransform().UpdateLocal();
      node->GetTransform().UpdateWorld();
      return true;
    });

    m_render_engine->RenderScene(m_scene_root.get());

    m_frame++;
  }

  void MainWindow::CreateVkInstance() {
    const VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Vulkan Playground",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Vulkan Playground Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_MAKE_VERSION(1, 0, 0)
    };

    std::vector<const char*> required_extension_names{};
    uint extension_count;
    SDL_Vulkan_GetInstanceExtensions(m_window, &extension_count, nullptr);
    required_extension_names.resize(extension_count);
    SDL_Vulkan_GetInstanceExtensions(m_window, &extension_count, required_extension_names.data());

    std::vector<const char*> required_layer_names{};
    if(enable_validation_layers && VulkanInstance::QueryInstanceLayerSupport("VK_LAYER_KHRONOS_validation")) {
      required_layer_names.push_back("VK_LAYER_KHRONOS_validation");
    }

    m_vk_instance = VulkanInstance::Create(app_info, required_extension_names, required_layer_names);
  }

  VulkanPhysicalDevice* MainWindow::PickPhysicalDevice() {
    std::optional<VulkanPhysicalDevice*> discrete_gpu;
    std::optional<VulkanPhysicalDevice*> integrated_gpu;

    for(auto& physical_device : m_vk_instance->EnumeratePhysicalDevices()) {
      switch(physical_device->GetProperties().deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:     discrete_gpu = physical_device.get(); break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: integrated_gpu = physical_device.get(); break;
        default: break;
      }
    }

    return discrete_gpu.value_or(integrated_gpu.value_or(nullptr));
  }

  void MainWindow::CreateLogicalDevice() {
    std::vector<const char*> required_device_layers{};

    if(enable_validation_layers) {
      const auto validation_layer_name = "VK_LAYER_KHRONOS_validation";

      if(m_vk_physical_device->QueryDeviceLayerSupport(validation_layer_name)) {
        required_device_layers.push_back(validation_layer_name);
      } else {
        ZEPHYR_WARN("Could not enable device validation layer");
      }
    }

    std::vector<const char*> required_device_extensions{"VK_KHR_swapchain"};

    for(auto extension_name : required_device_extensions) {
      if(!m_vk_physical_device->QueryDeviceExtensionSupport(extension_name)) {
        ZEPHYR_PANIC("Could not find device extension: {}", extension_name);
      }
    }

    if(m_vk_physical_device->QueryDeviceExtensionSupport("VK_KHR_portability_subset")) {
      required_device_extensions.push_back("VK_KHR_portability_subset");
    }

    std::optional<u32> graphics_plus_compute_queue_family_index;
    std::optional<u32> dedicated_compute_queue_family_index;

    // Figure out what queues we can create
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
    {
      /**
       * Info about queues present on the common vendors, gathered from:
       *   http://vulkan.gpuinfo.org/listreports.php
       *
       * Nvidia (up until Pascal (GTX 10XX)):
       *   - 16x graphics + compute + transfer + presentation
       *   -  1x transfer
       *
       * Nvidia (from Pascal (GTX 10XX) onwards):
       *   - 16x graphics + compute + transfer + presentation
       *   -  2x transfer
       *   -  8x compute + transfer + presentation (async compute?)
       *   -  1x transfer + video decode
       *
       * AMD:
       *   Seems to vary quite a bit from GPU to GPU, but usually have at least:
       *   - 1x graphics + compute + transfer + presentation
       *   - 1x compute + transfer + presentation (async compute?)
       *
       * Apple M1 (via MoltenVK):
       *   - 1x graphics + compute + transfer + presentation
       *
       * Intel:
       *   - 1x graphics + compute + transfer + presentation
       *
       * Furthermore the Vulkan spec guarantees that:
       *   - If an implementation exposes any queue family which supports graphics operation, then at least one
       *     queue family of at least one physical device exposed by the implementation must support graphics and compute operations.
       *
       *   - Queues which support graphics or compute commands implicitly always support transfer commands, therefore a
       *     queue family supporting graphics or compute commands might not explicitly report transfer capabilities, despite supporting them.
       *
       * Given this data, we chose to allocate the following queues:
       *   - 1x graphics + compute + transfer + presentation (required)
       *   - 1x compute + transfer + presentation (if present)
       */

      u32 queue_family_index = 0;

      for(const auto& queue_family : m_vk_physical_device->EnumerateQueueFamilies()) {
        const VkQueueFlags queue_flags = queue_family.queueFlags;

        /**
         * TODO: we require both our graphics + compute queue and our dedicated compute queues to support presentation.
         * But currently we do not do any checking to ensure that this is the case. From the looks of it,
         * it seems like this might require platform dependent code (see vkGetPhysicalDeviceWin32PresentationSupportKHR() for example).
         */
        switch(queue_flags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
          case VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT: {
            graphics_plus_compute_queue_family_index = queue_family_index;
            break;
          }
          case VK_QUEUE_COMPUTE_BIT: {
            dedicated_compute_queue_family_index = queue_family_index;
            break;
          }
        }

        queue_family_index++;
      }

      const f32 queue_priority = 0.0f;

      if(graphics_plus_compute_queue_family_index.has_value()) {
        queue_create_infos.push_back(VkDeviceQueueCreateInfo{
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .queueFamilyIndex = graphics_plus_compute_queue_family_index.value(),
          .queueCount = 1,
          .pQueuePriorities = &queue_priority
        });

        m_present_queue_family_indices.push_back(graphics_plus_compute_queue_family_index.value());
      } else {
        ZEPHYR_PANIC("Physical device does not have any graphics + compute queue");
      }

      if(dedicated_compute_queue_family_index.has_value()) {
        queue_create_infos.push_back(VkDeviceQueueCreateInfo{
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .queueFamilyIndex = dedicated_compute_queue_family_index.value(),
          .queueCount = 1,
          .pQueuePriorities = &queue_priority
        });

        m_present_queue_family_indices.push_back(dedicated_compute_queue_family_index.value());

        ZEPHYR_INFO("Got an asynchronous compute queue !");
      }
    }

    m_vk_device = m_vk_physical_device->CreateLogicalDevice(queue_create_infos, required_device_extensions);

    vkGetDeviceQueue(m_vk_device, graphics_plus_compute_queue_family_index.value(), 0u, &m_vk_graphics_compute_queue);

    if(dedicated_compute_queue_family_index.has_value()) {
      VkQueue queue;
      vkGetDeviceQueue(m_vk_device, dedicated_compute_queue_family_index.value(), 0u, &queue);
      m_vk_dedicated_compute_queue = queue;
    }
  }

  void MainWindow::CreateSurface() {
    if(!SDL_Vulkan_CreateSurface(m_window, m_vk_instance->Handle(), &m_vk_surface)) {
      ZEPHYR_PANIC("Failed to create a Vulkan surface for the window");
    }
  }

  void MainWindow::CreateRenderEngine() {
    m_render_engine = std::make_unique<RenderEngine>(CreateVulkanRenderBackend({
      .device  = m_vk_device,
      .surface = m_vk_surface,
      .graphics_compute_queue = m_vk_graphics_compute_queue,
      .present_queue_family_indices = m_present_queue_family_indices
    }));
  }

  void MainWindow::CreateScene() {
    m_scene_root = std::make_unique<SceneNode>();

    //std::shared_ptr<Material> pbr_material = std::make_shared<Material>(std::make_shared<PBRMaterialShader>());

    SceneNode* cube_a = m_scene_root->CreateChild("Cube A");
    //cube_a->CreateComponent<MeshComponent>(m_cube_mesh, pbr_material);
    cube_a->CreateComponent<MeshComponent>();
    cube_a->GetTransform().GetPosition() = Vector3{0.0f, 0.0f, -5.0f};

    SceneNode* cube_b = cube_a->CreateChild("Cube B");
    //cube_b->CreateComponent<MeshComponent>(m_cube_mesh, pbr_material);
    cube_b->CreateComponent<MeshComponent>();
    cube_b->GetTransform().GetScale() = Vector3{0.25f, 0.25f, 0.25f};
  }

  void MainWindow::Cleanup() {
    vkDeviceWaitIdle(m_vk_device);
    vkDestroySurfaceKHR(m_vk_instance->Handle(), m_vk_surface, nullptr);
    vkDestroyDevice(m_vk_device, nullptr);
    SDL_DestroyWindow(m_window);
  }

} // namespace zephyr