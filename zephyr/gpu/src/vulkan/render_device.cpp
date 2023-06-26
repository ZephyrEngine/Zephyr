
// Copyright (C) 2022 fleroviux. All rights reserved.

#include <zephyr/gpu/backend/vulkan.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "bind_group.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "command_pool.hpp"
#include "compute_pipeline.hpp"
#include "fence.hpp"
#include "pipeline_builder.hpp"
#include "pipeline_layout.hpp"
#include "queue.hpp"
#include "render_target.hpp"
#include "sampler.hpp"
#include "shader_module.hpp"
#include "texture.hpp"

namespace zephyr {

struct VulkanRenderDevice final : RenderDevice {
  explicit VulkanRenderDevice(VulkanRenderDeviceOptions const& options)
      : instance(options.instance)
      , physical_device(options.physical_device)
      , device(options.device)
      , queue_family_graphics(options.queue_family_graphics)
      , queue_family_compute(options.queue_family_compute) {
    CreateVmaAllocator();
    CreateDescriptorPool();
    CreateQueues();

    default_pipeline_layout = CreatePipelineLayout({});
  }

 ~VulkanRenderDevice() override {
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    vmaDestroyAllocator(allocator);
  }

  auto Handle() -> void* override {
    return (void*)device;
  }

  auto CreateBuffer(
    Buffer::Usage usage,
    Buffer::Flags flags,
    size_t size
  ) -> std::unique_ptr<Buffer> override {
    return std::make_unique<VulkanBuffer>(
      allocator,
      usage,
      flags,
      size
    );
  }

  auto CreateShaderModule(
    u32 const* spirv,
    size_t size
  ) -> std::shared_ptr<ShaderModule> override {
    return std::make_shared<VulkanShaderModule>(device, spirv, size);
  }

  auto CreateTexture2D(
    u32 width,
    u32 height,
    Texture::Format format,
    Texture::Usage usage,
    u32 mip_count = 1
  ) -> std::unique_ptr<Texture> override {
    return VulkanTexture::Create2D(device, allocator, width, height, mip_count, format, usage);
  }

  auto CreateTexture2DFromSwapchainImage(
    u32 width,
    u32 height,
    Texture::Format format,
    void* image_handle
  ) -> std::unique_ptr<Texture> override {
    return VulkanTexture::Create2DFromSwapchain(device, width, height, format, (VkImage)image_handle);
  }

  auto CreateTextureCube(
    u32 width,
    u32 height,
    Texture::Format format,
    Texture::Usage usage,
    u32 mip_count = 1
  ) -> std::unique_ptr<Texture> override {
    return VulkanTexture::CreateCube(device, allocator, width, height, mip_count, format, usage);
  }

  auto CreateSampler(
    Sampler::Config const& config
  ) -> std::unique_ptr<Sampler> override {
    return std::make_unique<VulkanSampler>(device, config);
  }

  auto DefaultNearestSampler() -> Sampler* override {
    if (!default_nearest_sampler) {
      default_nearest_sampler = CreateSampler(Sampler::Config{
        .mag_filter = Sampler::FilterMode::Nearest,
        .min_filter = Sampler::FilterMode::Nearest,
        .mip_filter = Sampler::FilterMode::Nearest
      });
    }

    return default_nearest_sampler.get();
  }

  auto DefaultLinearSampler() -> Sampler* override {
    if (!default_linear_sampler) {
      default_linear_sampler = CreateSampler(Sampler::Config{
        .mag_filter = Sampler::FilterMode::Linear,
        .min_filter = Sampler::FilterMode::Linear,
        .mip_filter = Sampler::FilterMode::Linear
      });
    }

    return default_linear_sampler.get();
  }

  auto CreateRenderTarget(
    std::span<Texture::View* const> color_attachments,
    Texture::View* depth_stencil_attachment = nullptr
  ) -> std::unique_ptr<RenderTarget> override {
    return std::make_unique<VulkanRenderTarget>(device, color_attachments, depth_stencil_attachment);
  }

  auto CreateRenderPassBuilder() -> std::unique_ptr<RenderPassBuilder> override {
    return std::make_unique<VulkanRenderPassBuilder>(device);
  }

  auto CreateBindGroupLayout(
    std::span<BindGroupLayout::Entry const> entries
  ) -> std::shared_ptr<BindGroupLayout> override {
    return std::make_shared<VulkanBindGroupLayout>(device, descriptor_pool, entries);
  }

  std::unique_ptr<BindGroup> CreateBindGroup(std::shared_ptr<BindGroupLayout> layout) override {
    return std::make_unique<VulkanBindGroup>(device, descriptor_pool, std::move(layout));
  }

  auto CreatePipelineLayout(
    std::span<BindGroupLayout* const> bind_group_layouts
  ) -> std::shared_ptr<PipelineLayout> override {
    return std::make_unique<VulkanPipelineLayout>(device, bind_group_layouts);
  }

  auto CreateGraphicsPipelineBuilder() -> std::unique_ptr<GraphicsPipelineBuilder> override {
    return std::make_unique<VulkanGraphicsPipelineBuilder>(device, default_pipeline_layout);
  }

  auto CreateComputePipeline(
    ShaderModule* shader_module,
    std::shared_ptr<PipelineLayout> layout
  ) -> std::unique_ptr<ComputePipeline> override {
    return VulkanComputePipeline::Create(device, shader_module, layout);
  }

  auto CreateGraphicsCommandPool(CommandPool::Usage usage) -> std::unique_ptr<CommandPool> override {
    return std::make_unique<VulkanCommandPool>(device, queue_family_graphics, usage);
  }

  auto CreateComputeCommandPool(CommandPool::Usage usage) -> std::unique_ptr<CommandPool> override {
    return std::make_unique<VulkanCommandPool>(device, queue_family_compute, usage);
  }

  auto CreateCommandBuffer(CommandPool* pool) -> std::unique_ptr<CommandBuffer> override {
    return std::make_unique<VulkanCommandBuffer>(device, pool);
  }

  auto CreateFence(Fence::CreateSignalled create_signalled) -> std::unique_ptr<Fence> override {
    return std::make_unique<VulkanFence>(device, create_signalled);
  }

  auto GraphicsQueue() -> Queue* override {
    return graphics_queue.get();
  }

  auto ComputeQueue() -> Queue* override {
    return compute_queue.get();
  }

  void WaitIdle() override {
    vkDeviceWaitIdle(device);
  }

private:
  void CreateVmaAllocator() {
    auto info = VmaAllocatorCreateInfo{};
    info.flags = 0;
    info.physicalDevice = physical_device;
    info.device = device;
    info.preferredLargeHeapBlockSize = 0;
    info.pAllocationCallbacks = nullptr;
    info.pDeviceMemoryCallbacks = nullptr;
    info.pHeapSizeLimit = nullptr;
    info.pVulkanFunctions = nullptr;
    info.instance = instance;
    info.vulkanApiVersion = VK_API_VERSION_1_2;
    info.pTypeExternalMemoryHandleTypes = nullptr;

    if (vmaCreateAllocator(&info, &allocator) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanRenderDevice: failed to create the VMA allocator");
    }
  }

  void CreateDescriptorPool() {
    // @todo: create pools for other descriptor types
    VkDescriptorPoolSize pool_sizes[] {
      {
        .type = VK_DESCRIPTOR_TYPE_SAMPLER,
        .descriptorCount = 4096
      },
      {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 4096
      },
      {
        .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .descriptorCount = 4096
      },
      {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 4096
      },
      {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 4096
      }
    };

    auto info = VkDescriptorPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 4096,
      .poolSizeCount = (u32)(sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize)),
      .pPoolSizes = pool_sizes
    };

    if (vkCreateDescriptorPool(device, &info, nullptr, &descriptor_pool) != VK_SUCCESS) {
      ZEPHYR_PANIC("VulkanRenderDevice: failed to create descriptor pool");
    }
  }

  void CreateQueues() {
    graphics_queue = CreateQueueForFamily(queue_family_graphics);

    if(queue_family_graphics != queue_family_compute) {
      compute_queue = CreateQueueForFamily(queue_family_compute);
    } else {
      compute_queue = graphics_queue;
    }
  }

  auto CreateQueueForFamily(u32 queue_family_index) -> std::unique_ptr<VulkanQueue> {
    VkQueue queue;

    vkGetDeviceQueue(device, queue_family_index, 0, &queue);

    return std::make_unique<VulkanQueue>(queue);
  }

  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkDescriptorPool descriptor_pool;
  VmaAllocator allocator;
  std::shared_ptr<VulkanQueue> graphics_queue;
  std::shared_ptr<VulkanQueue> compute_queue;
  u32 queue_family_graphics;
  u32 queue_family_compute;

  std::shared_ptr<PipelineLayout> default_pipeline_layout;
  std::unique_ptr<Sampler> default_nearest_sampler;
  std::unique_ptr<Sampler> default_linear_sampler;
};

auto CreateVulkanRenderDevice(
  VulkanRenderDeviceOptions const& options
) -> std::unique_ptr<RenderDevice> {
  return std::make_unique<VulkanRenderDevice>(options);
}

} // namespace zephyr
