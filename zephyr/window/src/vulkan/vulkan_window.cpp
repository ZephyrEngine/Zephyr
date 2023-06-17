
#include <algorithm>
#include <zephyr/logger/logger.hpp>
#include <zephyr/panic.hpp>
#include <SDL_vulkan.h>

#include "vulkan_window.hpp"

namespace zephyr {

  VulkanWindow::VulkanWindow() {
    CreateSDL2Window();
    CreateVulkanContext();
  }

  void VulkanWindow::SetWindowTitle(std::string_view window_title) {
    SDL_SetWindowTitle(window, window_title.data());
  }

  auto VulkanWindow::GetSize() -> Size {
    Size size{};

    SDL_GetWindowSize(window, &size.width, &size.height);
    return size;
  }

  void VulkanWindow::SetSize(int width, int height) {
    SDL_SetWindowSize(window, width, height);
  }

  void VulkanWindow::CreateSDL2Window() {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
      "",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      1600,
      900,
      SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
  }

  void VulkanWindow::CreateVulkanContext() {
    CreateVulkanInstance();

    vulkan_physical_device = PickPhysicalDevice();

    if (vulkan_physical_device == VK_NULL_HANDLE) {
      ZEPHYR_PANIC("VulkanWindow: failed to find a suitable GPU");
    }

    CreateLogicalDevice();
    CreateSurface();
    CreateRenderDevice();
    CreateSwapChain();
  }

  void VulkanWindow::CreateVulkanInstance() {
    const VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Zephyr",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Zephyr",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_MAKE_VERSION(1, 2, 189)
    };

    const auto extensions = GetInstanceExtensions();
    const auto layers = GetInstanceLayers();

    const VkInstanceCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pApplicationInfo = &app_info,
      .enabledLayerCount = static_cast<u32>(layers.size()),
      .ppEnabledLayerNames = layers.data(),
      .enabledExtensionCount = static_cast<u32>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()
    };

    if(vkCreateInstance(&create_info, nullptr, &vulkan_instance) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanWindow: failed to create a Vulkan instance");
    }
  }

  void VulkanWindow::CreateLogicalDevice() {
    const std::vector<char const*> k_desired_device_extensions{
      "VK_KHR_swapchain",
#ifdef __APPLE__
      "VK_KHR_portability_subset"
#endif
    };

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(vulkan_physical_device, &physical_device_properties);
    ZEPHYR_INFO("VulkanWindow: found GPU: {}", physical_device_properties.deviceName);

    std::vector<char const*> device_layers = GetDeviceLayers();

    VkPhysicalDeviceFeatures available_device_features;
    vkGetPhysicalDeviceFeatures(vulkan_physical_device, &available_device_features);

    std::vector<VkDeviceQueueCreateInfo> queue_create_info;

    const float queue_priority = 0.0;

    const auto PushQueueCreateInfo = [&](u32 queue_family_index) {
      queue_create_info.push_back({
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
      });
    };

    u32 queue_family_count;
    std::vector<VkQueueFamilyProperties> queue_family_properties;
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_physical_device, &queue_family_count, nullptr);
    queue_family_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_physical_device, &queue_family_count, queue_family_properties.data());

    for(u32 i = 0; i < queue_family_count; i++) {
      VkQueueFamilyProperties& properties = queue_family_properties[i];

      if(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        queue_family_graphics = i;
      }

      if(properties.queueFlags & VK_QUEUE_COMPUTE_BIT) {
        queue_family_compute = i;
      }

      if(queue_family_graphics.has_value() && queue_family_compute.has_value()) {
        break;
      }
    }

    if(!queue_family_graphics.has_value()) {
      ZEPHYR_PANIC("VulkanWindow: failed to find a graphics-capable queue family");
    }

    if(!queue_family_compute.has_value()) {
      ZEPHYR_PANIC("VulkanWindow: failed to find a compute-capable queue family");
    }

    PushQueueCreateInfo(queue_family_graphics.value());

    if(queue_family_graphics != queue_family_compute) {
      PushQueueCreateInfo(queue_family_compute.value());
    }

    VkDeviceCreateInfo device_create_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueCreateInfoCount = (u32)queue_create_info.size(),
      .pQueueCreateInfos = queue_create_info.data(),
      .enabledLayerCount = (u32)device_layers.size(),
      .ppEnabledLayerNames = device_layers.data(),
      .enabledExtensionCount = (u32)k_desired_device_extensions.size(),
      .ppEnabledExtensionNames = k_desired_device_extensions.data()
    };

    if(vkCreateDevice(vulkan_physical_device, &device_create_info, nullptr, &vulkan_device) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanWindow: failed to create a logical device");
    }
  }

  std::vector<char const*> VulkanWindow::GetInstanceExtensions() {
    u32 extension_count;
    std::vector<char const*> extensions;

    SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr);
    extensions.resize(extension_count);
    SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data());

    return extensions;
  }

  std::vector<char const*> VulkanWindow::GetInstanceLayers() {
    const std::vector<char const*> k_desired_instance_layers {
#if !defined(NDEBUG)
      "VK_LAYER_KHRONOS_validation"
#endif
    };

    std::vector<char const*> result;

    u32 instance_layer_count;
    std::vector<VkLayerProperties> instance_layers;

    vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
    instance_layers.resize(instance_layer_count);
    vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.data());

    for(auto layer_name : k_desired_instance_layers) {
      const auto predicate = [&](VkLayerProperties& instance_layer) {
        return std::strcmp(layer_name, instance_layer.layerName) == 0;
      };

      auto match = std::find_if(instance_layers.begin(), instance_layers.end(), predicate);

      if(match != instance_layers.end()) {
        result.push_back(layer_name);
      }
    }

    return result;
  }

  VkPhysicalDevice VulkanWindow::PickPhysicalDevice() {
    u32 physical_device_count;
    std::vector<VkPhysicalDevice> physical_devices;

    vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, nullptr);
    physical_devices.resize(physical_device_count);
    vkEnumeratePhysicalDevices(
      vulkan_instance, &physical_device_count, physical_devices.data());

    VkPhysicalDevice fallback_gpu = VK_NULL_HANDLE;

    /* TODO: implement a more robust logic to pick the best device.
     * At the moment we pick the first discrete GPU we discover.
     * If no discrete GPU is available we fallback to any integrated GPU.
     */
    for(auto physical_device : physical_devices) {
      VkPhysicalDeviceProperties physical_device_props;

      vkGetPhysicalDeviceProperties(physical_device, &physical_device_props);

      auto device_type = physical_device_props.deviceType;

      if(device_type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        return physical_device;
      }

      if(device_type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        fallback_gpu = physical_device;
      }
    }

    return fallback_gpu;
  }

  std::vector<char const*> VulkanWindow::GetDeviceLayers() {
    const std::vector<char const*> k_desired_device_layers{
#if !defined(NDEBUG)
      "VK_LAYER_KHRONOS_validation"
#endif
    };

    std::vector<char const*> result;

    u32 device_layer_count;
    std::vector<VkLayerProperties> device_layers;
    vkEnumerateDeviceLayerProperties(vulkan_physical_device, &device_layer_count, nullptr);
    device_layers.resize(device_layer_count);
    vkEnumerateDeviceLayerProperties(vulkan_physical_device, &device_layer_count, device_layers.data());

    for(auto layer_name : k_desired_device_layers) {
      const auto predicate = [&](VkLayerProperties& device_layer) {
        return std::strcmp(layer_name, device_layer.layerName) == 0;
      };

      const auto match = std::find_if(device_layers.begin(), device_layers.end(), predicate);

      if(match != device_layers.end()) {
        result.push_back(layer_name);
      }
    }

    return result;
  }

  void VulkanWindow::CreateSurface() {
    if (!SDL_Vulkan_CreateSurface(window, vulkan_instance, &vulkan_surface)) {
      ZEPHYR_PANIC("VulkanWindow: failed to create Vulkan surface");
    }

    VkBool32 supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(vulkan_physical_device, queue_family_graphics.value(), vulkan_surface, &supported);

    if (supported == VK_FALSE) {
      ZEPHYR_PANIC("VulkanWindow: physical device or queue family cannot present to surface");
    }
  }

  void VulkanWindow::CreateRenderDevice() {
    render_device = CreateVulkanRenderDevice({
      vulkan_instance,
      vulkan_physical_device,
      vulkan_device,
      queue_family_graphics.value(),
      queue_family_compute.value()
    });
  }

  void VulkanWindow::CreateSwapChain() {
    const auto size = GetSize();

    swap_chain = std::make_shared<VulkanSwapChain>(
      render_device, vulkan_surface, size.width, size.height);
  }

  bool VulkanWindow::PollEvents() {
    SDL_Event event;

    if(!did_fire_on_resize_atleast_once) {
      /* We cannot do this inside the constructor,
       * as Window::OnResize() is virtual and the vtable would not be valid at that point.
       */
      HandleWindowSizeChanged();
    }

    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) {
        return false;
      }

      if((event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
        HandleWindowSizeChanged();
      }
    }

    return true;
  }

  void VulkanWindow::SetOnResizeCallback(std::function<void(int, int)> callback) {
    on_resize_cb = callback;
  }

  void VulkanWindow::HandleWindowSizeChanged() {
    int width;
    int height;

    SDL_GetWindowSize(window, &width, &height);

    ((VulkanSwapChain*)swap_chain.get())->SetSize(width, height);

    on_resize_cb(width, height);

    did_fire_on_resize_atleast_once = true;
  }

} // namespace zephyr