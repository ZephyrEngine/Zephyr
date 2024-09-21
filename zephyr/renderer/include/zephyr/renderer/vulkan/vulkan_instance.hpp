
#pragma once

#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <algorithm>
#include <cstring>
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_physical_device.hpp"

namespace zephyr {

class VulkanInstance {
  public:
   ~VulkanInstance() {
      vkDestroyInstance(m_vk_instance, nullptr);
    }

    [[nodiscard]] VkInstance Handle() {
      return m_vk_instance;
    }

    static bool QueryInstanceExtensionSupport(const char* extension_name) {
      const auto predicate = [&](const VkExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, extension_name) == 0;
      };
      return std::ranges::find_if(k_available_vk_instance_extensions, predicate) != k_available_vk_instance_extensions.end();
    }

    static bool QueryInstanceLayerSupport(const char* layer_name) {
      const auto predicate = [&](const VkLayerProperties& layer) {
        return std::strcmp(layer.layerName, layer_name) == 0;
      };
      return std::ranges::find_if(k_available_vk_instance_layers, predicate) != k_available_vk_instance_layers.end();
    }

    [[nodiscard]] std::span<const std::unique_ptr<VulkanPhysicalDevice>> EnumeratePhysicalDevices() const {
      return m_vk_physical_devices;
    }

    static std::unique_ptr<VulkanInstance> Create(
      const VkApplicationInfo& app_info,
      std::vector<const char*> required_instance_extensions,
      std::vector<const char*> required_instance_layers
    ) {
      for(const auto required_layer_name : required_instance_layers) {
        if(!QueryInstanceLayerSupport(required_layer_name)) {
          ZEPHYR_PANIC("Could not find required Vulkan instance layer: {}", required_layer_name);
        }
      }

      VkInstanceCreateFlags instance_create_flags = 0;

      // TODO(fleroviux): make this optional
      if(QueryInstanceExtensionSupport(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
        required_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        required_instance_extensions.push_back("VK_KHR_get_physical_device_properties2"); // required by VK_KHR_portability_subset
        instance_create_flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
      }

      for(const auto required_extension_name : required_instance_extensions) {
        if(!QueryInstanceExtensionSupport(required_extension_name)) {
          ZEPHYR_PANIC("Could not find required Vulkan instance extension: {}", required_extension_name);
        }
      }

      const VkInstanceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = instance_create_flags,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = (u32)required_instance_layers.size(),
        .ppEnabledLayerNames = required_instance_layers.data(),
        .enabledExtensionCount = (u32)required_instance_extensions.size(),
        .ppEnabledExtensionNames = required_instance_extensions.data()
      };

      VkInstance vk_instance{};

      // TODO(fleroviux): pass error to the user instead of asserting.
      if(vkCreateInstance(&create_info, nullptr, &vk_instance) != VK_SUCCESS) {
        ZEPHYR_PANIC("Failed to create Vulkan instance!");
      }
      return std::unique_ptr<VulkanInstance>{new VulkanInstance{vk_instance}};
    }

  private:
    explicit VulkanInstance(VkInstance vk_instance) : m_vk_instance{vk_instance} {
      PopulatePhysicalDeviceList();
    }

    void PopulatePhysicalDeviceList() {
      u32 device_count;
      std::vector<VkPhysicalDevice> vk_physical_devices{};
      vkEnumeratePhysicalDevices(m_vk_instance, &device_count, nullptr);
      vk_physical_devices.resize(device_count);
      vkEnumeratePhysicalDevices(m_vk_instance, &device_count, vk_physical_devices.data());

      for(VkPhysicalDevice physical_device : vk_physical_devices) {
        m_vk_physical_devices.emplace_back(std::make_unique<VulkanPhysicalDevice>(physical_device));
      }
    }

    VkInstance m_vk_instance{};
    std::vector<std::unique_ptr<VulkanPhysicalDevice>> m_vk_physical_devices{};

    static std::vector<VkExtensionProperties> k_available_vk_instance_extensions;
    static std::vector<VkLayerProperties> k_available_vk_instance_layers;
};

} // namespace zephyr
