
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

  class VulkanRenderDevice final : public RenderDevice {
    public:
      explicit VulkanRenderDevice(const VulkanRenderDeviceOptions& options)
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

      void* Handle() override {
        return (void*)device;
      }

      std::unique_ptr<Buffer> CreateBuffer(Buffer::Usage usage, Buffer::Flags flags, size_t size) override {
        return std::make_unique<VulkanBuffer>(allocator, usage, flags, size);
      }

      std::shared_ptr<ShaderModule> CreateShaderModule(const u32* spirv, size_t size) override {
        return std::make_shared<VulkanShaderModule>(device, spirv, size);
      }

      std::unique_ptr<Texture> CreateTexture2D(
        u32 width,
        u32 height,
        Texture::Format format,
        Texture::Usage usage,
        u32 mip_count = 1
      ) override {
        return VulkanTexture::Create2D(device, allocator, width, height, mip_count, format, usage);
      }

      std::unique_ptr<Texture> CreateTexture2DFromSwapchainImage(
        u32 width,
        u32 height,
        Texture::Format format,
        void* image_handle
      ) override {
        return VulkanTexture::Create2DFromSwapchain(device, width, height, format, (VkImage)image_handle);
      }

      std::unique_ptr<Texture> CreateTextureCube(
        u32 width,
        u32 height,
        Texture::Format format,
        Texture::Usage usage,
        u32 mip_count = 1
      ) override {
        return VulkanTexture::CreateCube(device, allocator, width, height, mip_count, format, usage);
      }

      std::unique_ptr<Sampler> CreateSampler(const Sampler::Config& config) {
        return std::make_unique<VulkanSampler>(device, config);
      }

      Sampler* DefaultNearestSampler() override {
        if (!default_nearest_sampler) {
          default_nearest_sampler = CreateSampler(Sampler::Config{
            .mag_filter = Sampler::FilterMode::Nearest,
            .min_filter = Sampler::FilterMode::Nearest,
            .mip_filter = Sampler::FilterMode::Nearest
          });
        }

        return default_nearest_sampler.get();
      }

      Sampler* DefaultLinearSampler() override {
        if (!default_linear_sampler) {
          default_linear_sampler = CreateSampler(Sampler::Config{
            .mag_filter = Sampler::FilterMode::Linear,
            .min_filter = Sampler::FilterMode::Linear,
            .mip_filter = Sampler::FilterMode::Linear
          });
        }

        return default_linear_sampler.get();
      }

      std::unique_ptr<RenderTarget> CreateRenderTarget(
        std::span<Texture::View* const> color_attachments,
        Texture::View* depth_stencil_attachment = nullptr
      ) override {
        return std::make_unique<VulkanRenderTarget>(device, color_attachments, depth_stencil_attachment);
      }

      std::unique_ptr<RenderPassBuilder> CreateRenderPassBuilder() override {
        return std::make_unique<VulkanRenderPassBuilder>(device);
      }

      auto CreateBindGroupLayout(std::span<const BindGroupLayout::Entry> entries) -> std::shared_ptr<BindGroupLayout> override {
        return std::make_shared<VulkanBindGroupLayout>(device, descriptor_pool, entries);
      }

      std::unique_ptr<BindGroup> CreateBindGroup(std::shared_ptr<BindGroupLayout> layout) override {
        return std::make_unique<VulkanBindGroup>(device, descriptor_pool, std::move(layout));
      }

      std::shared_ptr<PipelineLayout> CreatePipelineLayout(std::span<BindGroupLayout* const> bind_group_layouts) override {
        return std::make_unique<VulkanPipelineLayout>(device, bind_group_layouts);
      }

      std::unique_ptr<GraphicsPipelineBuilder> CreateGraphicsPipelineBuilder() override {
        return std::make_unique<VulkanGraphicsPipelineBuilder>(device, default_pipeline_layout);
      }

      std::unique_ptr<ComputePipeline> CreateComputePipeline(
        ShaderModule* shader_module,
        std::shared_ptr<PipelineLayout> layout
      ) override {
        return VulkanComputePipeline::Create(device, shader_module, layout);
      }

      std::shared_ptr<CommandPool> CreateGraphicsCommandPool(CommandPool::Usage usage) override {
        return std::make_shared<VulkanCommandPool>(device, queue_family_graphics, usage);
      }

      std::shared_ptr<CommandPool> CreateComputeCommandPool(CommandPool::Usage usage) override {
        return std::make_shared<VulkanCommandPool>(device, queue_family_compute, usage);
      }

      std::unique_ptr<CommandBuffer> CreateCommandBuffer(std::shared_ptr<CommandPool> pool) override {
        return std::make_unique<VulkanCommandBuffer>(device, std::move(pool));
      }

      std::unique_ptr<Fence> CreateFence(Fence::CreateSignalled create_signalled) override {
        return std::make_unique<VulkanFence>(device, create_signalled);
      }

      Queue* GraphicsQueue() override {
        return graphics_queue.get();
      }

      Queue* ComputeQueue() override {
        return compute_queue.get();
      }

      void WaitIdle() override {
        vkDeviceWaitIdle(device);
      }

    private:
      void CreateVmaAllocator() {
        VmaAllocatorCreateInfo info{
          .flags = 0,
          .physicalDevice = physical_device,
          .device = device,
          .preferredLargeHeapBlockSize = 0,
          .pAllocationCallbacks = nullptr,
          .pDeviceMemoryCallbacks = nullptr,
          .pHeapSizeLimit = nullptr,
          .pVulkanFunctions = nullptr,
          .instance = instance,
          .vulkanApiVersion = VK_API_VERSION_1_2,
          .pTypeExternalMemoryHandleTypes = nullptr
        };

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

        const VkDescriptorPoolCreateInfo info{
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

      std::unique_ptr<VulkanQueue> CreateQueueForFamily(u32 queue_family_index) {
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

  std::unique_ptr<RenderDevice> CreateVulkanRenderDevice(const VulkanRenderDeviceOptions& options) {
    return std::make_unique<VulkanRenderDevice>(options);
  }

} // namespace zephyr
