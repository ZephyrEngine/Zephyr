
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

    {
      m_vk_physical_device = PickPhysicalDevice();

      if(m_vk_physical_device == VK_NULL_HANDLE) {
        ZEPHYR_PANIC("Failed to find a suitable GPU!");
      }

      VkPhysicalDeviceProperties physical_device_props{};
      vkGetPhysicalDeviceProperties(m_vk_physical_device, &physical_device_props);
      ZEPHYR_INFO("GPU: {}", physical_device_props.deviceName);
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

    std::vector<const char*> instance_layers{
    };
    {
      if(enable_validation_layers) {
        const auto validation_layer_name = "VK_LAYER_KHRONOS_validation";

        u32 layer_count;
        std::vector<VkLayerProperties> available_layers{};

        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        available_layers.resize(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        const auto predicate =
          [&](const VkLayerProperties& layer_props) {
            return std::strcmp(layer_props.layerName, validation_layer_name) == 0; };

        for(auto& prop : available_layers) ZEPHYR_INFO("{}", prop.layerName);

        if(std::ranges::find_if(available_layers, predicate) != available_layers.end()) {
          instance_layers.push_back(validation_layer_name);
        } else {
          ZEPHYR_WARN("Could not enable instance validation layer");
        }
      }
    }

    // Collect all instance extensions that we need
    std::vector<const char*> required_extension_names{};
    {
      uint extension_count;
      SDL_Vulkan_GetInstanceExtensions(m_window, &extension_count, nullptr);
      required_extension_names.resize(extension_count);
      SDL_Vulkan_GetInstanceExtensions(m_window, &extension_count, required_extension_names.data());
    }

    VkInstanceCreateFlags instance_create_flags = 0;

    // Validate that all required extensions are present:
    {
      u32 extension_count;
      std::vector<VkExtensionProperties> available_extensions{};

      vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
      available_extensions.resize(extension_count);
      vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

      if(std::find_if(
          available_extensions.begin(), available_extensions.end(), [](const VkExtensionProperties& extension) {
            return std::strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0;
          }) != available_extensions.end()) {
        required_extension_names.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        required_extension_names.push_back("VK_KHR_get_physical_device_properties2"); // required by VK_KHR_portability_subset
        instance_create_flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
      }

      for(const auto required_extension_name : required_extension_names) {
        const auto predicate =
          [&](const VkExtensionProperties& extensions_props) {
            return std::strcmp(extensions_props.extensionName, required_extension_name) == 0; };

        if(std::ranges::find_if(available_extensions, predicate) == available_extensions.end()) {
          ZEPHYR_PANIC("Could not find required Vulkan instance extension: {}", required_extension_name);
        }
      }
    }

    const VkInstanceCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = instance_create_flags,
      .pApplicationInfo = &app_info,
      .enabledLayerCount = (u32)instance_layers.size(),
      .ppEnabledLayerNames = instance_layers.data(),
      .enabledExtensionCount = (u32)required_extension_names.size(),
      .ppEnabledExtensionNames = required_extension_names.data()
    };

    if(vkCreateInstance(&create_info, nullptr, &m_vk_instance) != VK_SUCCESS) {
      ZEPHYR_PANIC("Failed to create Vulkan instance!");
    }
  }

  VkPhysicalDevice MainWindow::PickPhysicalDevice() {
    u32 device_count;
    std::vector<VkPhysicalDevice> physical_devices{};

    vkEnumeratePhysicalDevices(m_vk_instance, &device_count, nullptr);
    physical_devices.resize(device_count);
    vkEnumeratePhysicalDevices(m_vk_instance, &device_count, physical_devices.data());

    std::optional<VkPhysicalDevice> discrete_gpu;
    std::optional<VkPhysicalDevice> integrated_gpu;

    for(VkPhysicalDevice physical_device : physical_devices) {
      VkPhysicalDeviceProperties physical_device_props;

      vkGetPhysicalDeviceProperties(physical_device, &physical_device_props);

      // TODO: pick a suitable GPU based on score.
      // TODO: check if the GPU supports all the features that our render backend will need.

      if(physical_device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        discrete_gpu = physical_device;
      }

      if(physical_device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        integrated_gpu = physical_device;
      }
    }

    return discrete_gpu.value_or(integrated_gpu.value_or((VkPhysicalDevice)VK_NULL_HANDLE));
  }

  void MainWindow::CreateLogicalDevice() {
    // This is pretty much the same logic that we also used for instance layers
    std::vector<const char*> device_layers{
    };
    {
      if(enable_validation_layers) {
        const auto validation_layer_name = "VK_LAYER_KHRONOS_validation";

        u32 layer_count;
        std::vector<VkLayerProperties> available_layers{};

        vkEnumerateDeviceLayerProperties(m_vk_physical_device, &layer_count, nullptr);
        available_layers.resize(layer_count);
        vkEnumerateDeviceLayerProperties(m_vk_physical_device, &layer_count, available_layers.data());

        const auto predicate =
          [&](const VkLayerProperties& layer_props) {
            return std::strcmp(layer_props.layerName, validation_layer_name) == 0; };

        for(auto& prop : available_layers) ZEPHYR_INFO("{}", prop.layerName);

        if(std::ranges::find_if(available_layers, predicate) != available_layers.end()) {
          device_layers.push_back(validation_layer_name);
        } else {
          ZEPHYR_WARN("Could not enable device validation layer");
        }
      }
    }

    std::vector<const char*> required_extensions{
      "VK_KHR_swapchain"
    };

    u32 extension_count;
    std::vector<VkExtensionProperties> available_extensions{};
    vkEnumerateDeviceExtensionProperties(m_vk_physical_device, nullptr, &extension_count, nullptr);
    available_extensions.resize(extension_count);
    vkEnumerateDeviceExtensionProperties(m_vk_physical_device, nullptr, &extension_count, available_extensions.data());

    // TODO: validate that device extensions are present.

    if(std::find_if(
        available_extensions.begin(), available_extensions.end(), [](const VkExtensionProperties& extension) {
          return std::strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0;
        }) != available_extensions.end()) {
      required_extensions.push_back("VK_KHR_portability_subset");
    }

    std::optional<u32> graphics_plus_compute_queue_family_index;
    std::optional<u32> dedicated_compute_queue_family_index;

    // Figure out what queues we can create
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
    {
      u32 queue_family_count;
      std::vector<VkQueueFamilyProperties> queue_family_props{};

      vkGetPhysicalDeviceQueueFamilyProperties(m_vk_physical_device, &queue_family_count, nullptr);
      queue_family_props.resize(queue_family_count);
      vkGetPhysicalDeviceQueueFamilyProperties(m_vk_physical_device, &queue_family_count, queue_family_props.data());

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

      for(u32 queue_family_index = 0; queue_family_index < queue_family_count; queue_family_index++) {
        const auto& queue_family = queue_family_props[queue_family_index];
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

    const VkDeviceCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueCreateInfoCount = (u32)queue_create_infos.size(),
      .pQueueCreateInfos = queue_create_infos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = (u32)required_extensions.size(),
      .ppEnabledExtensionNames = required_extensions.data(),
      .pEnabledFeatures = nullptr
    };

    if(vkCreateDevice(m_vk_physical_device, &create_info, nullptr, &m_vk_device) != VK_SUCCESS) {
      ZEPHYR_PANIC("Failed to create a logical device");
    }

    vkGetDeviceQueue(m_vk_device, graphics_plus_compute_queue_family_index.value(), 0u, &m_vk_graphics_compute_queue);

    if(dedicated_compute_queue_family_index.has_value()) {
      VkQueue queue;
      vkGetDeviceQueue(m_vk_device, dedicated_compute_queue_family_index.value(), 0u, &queue);
      m_vk_dedicated_compute_queue = queue;
    }
  }

  void MainWindow::CreateSurface() {
    if(!SDL_Vulkan_CreateSurface(m_window, m_vk_instance, &m_vk_surface)) {
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
    vkDestroySurfaceKHR(m_vk_instance, m_vk_surface, nullptr);
    vkDestroyDevice(m_vk_device, nullptr);
    vkDestroyInstance(m_vk_instance, nullptr);
    SDL_DestroyWindow(m_window);
  }

} // namespace zephyr