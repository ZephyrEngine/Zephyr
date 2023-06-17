
#include <zephyr/logger/logger.hpp>
#include <zephyr/panic.hpp>

#include "vulkan_swap_chain.hpp"

namespace zephyr {

  VulkanSwapChain::VulkanSwapChain(
    std::shared_ptr<RenderDevice> render_device,
    VkSurfaceKHR surface,
    int width,
    int height
  )   : render_device{std::move(render_device)}, surface{surface} {
    SetSize(width, height);
    fence = this->render_device->CreateFence();
  }

  std::shared_ptr<RenderTarget>& VulkanSwapChain::AcquireNextRenderTarget() {
    VkDevice device = (VkDevice)render_device->Handle();

    u32 image_id; // current_image_id is std::optional<u32>
    fence->Reset();
    vkAcquireNextImageKHR(device, swap_chain, ~0ULL, VK_NULL_HANDLE, (VkFence)fence->Handle(), &image_id);
    fence->Wait();

    current_image_id = image_id;

    return render_targets[image_id];
  }

  void VulkanSwapChain::Present() {
    if (!current_image_id) {
      ZEPHYR_PANIC("VulkanSwapChain: called Present() without acquiring an image first");
    }

    VkResult result;
    u32 image_id = current_image_id.value();

    const VkPresentInfoKHR present_info{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .swapchainCount = 1,
      .pSwapchains = &swap_chain,
      .pImageIndices = &image_id,
      .pResults = &result
    };

    vkQueuePresentKHR((VkQueue)render_device->GraphicsQueue()->Handle(), &present_info);
  }

  void VulkanSwapChain::SetSize(int width, int height) {
    this->width = width;
    this->height = height;

    Create();
  }

  void VulkanSwapChain::Create() {
    CreateSwapChain();
    CreateRenderTargets();
  }

  void VulkanSwapChain::CreateSwapChain() {
    const VkExtent2D extent{
      .width = (u32)width,
      .height = (u32)height
    };

    const VkSwapchainCreateInfoKHR create_info{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .surface = surface,
      .minImageCount = 2,
      .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
      .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = swap_chain
    };

    if (vkCreateSwapchainKHR((VkDevice)render_device->Handle(), &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanWindow: failed to create a swap chain");
    }
  }

  void VulkanSwapChain::CreateRenderTargets() {
    u32 image_count;
    std::vector<VkImage> images;

    vkGetSwapchainImagesKHR((VkDevice)render_device->Handle(), swap_chain, &image_count, nullptr);
    images.resize(image_count);
    vkGetSwapchainImagesKHR((VkDevice)render_device->Handle(), swap_chain, &image_count, images.data());

    color_textures.clear();
    render_targets.clear();

    // @todo: use the correct texture resolution
    // @todo: let the user specify whether they need a depth buffer or not.
    for (auto image : images) {
      auto color_attachment = render_device->CreateTexture2DFromSwapchainImage(
        (u32)width, (u32)height, Texture::Format::B8G8R8A8_SRGB, (void*)image);

      auto depth_attachment = render_device->CreateTexture2D(
        (u32)width, (u32)height, Texture::Format::DEPTH_U16, Texture::Usage::DepthStencilAttachment);

      render_targets.push_back(render_device->CreateRenderTarget({{color_attachment->DefaultView()}}, depth_attachment->DefaultView()));

      color_textures.push_back(std::move(color_attachment));
      depth_textures.push_back(std::move(depth_attachment));
    }

    ZEPHYR_INFO("Swap Chain render targets successfully created");
  }

} // namespace zephyr
