
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
          : m_instance{options.instance}
          , m_physical_device{options.physical_device}
          , m_device{options.device}
          , m_queue_family_graphics{options.queue_family_graphics}
          , m_queue_family_compute{options.queue_family_compute} {
        CreateVmaAllocator();
        CreateDescriptorPool();
        CreateQueues();

        m_default_pipeline_layout = CreatePipelineLayout({});
      }

     ~VulkanRenderDevice() override {
        vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);
        vmaDestroyAllocator(m_allocator);
      }

      void* Handle() override {
        return (void*)m_device;
      }

      std::unique_ptr<Buffer> CreateBuffer(Buffer::Usage usage, Buffer::Flags flags, size_t size) override {
        return std::make_unique<VulkanBuffer>(m_allocator, usage, flags, size);
      }

      std::shared_ptr<ShaderModule> CreateShaderModule(const u32* spirv, size_t size) override {
        return std::make_shared<VulkanShaderModule>(m_device, spirv, size);
      }

      std::unique_ptr<Texture> CreateTexture2D(
        u32 width,
        u32 height,
        Texture::Format format,
        Texture::Usage usage,
        u32 mip_count = 1
      ) override {
        return VulkanTexture::Create2D(m_device, m_allocator, width, height, mip_count, format, usage);
      }

      std::unique_ptr<Texture> CreateTexture2DFromSwapchainImage(
        u32 width,
        u32 height,
        Texture::Format format,
        void* image_handle
      ) override {
        return VulkanTexture::Create2DFromSwapchain(m_device, width, height, format, (VkImage)image_handle);
      }

      std::unique_ptr<Texture> CreateTextureCube(
        u32 width,
        u32 height,
        Texture::Format format,
        Texture::Usage usage,
        u32 mip_count = 1
      ) override {
        return VulkanTexture::CreateCube(m_device, m_allocator, width, height, mip_count, format, usage);
      }

      std::unique_ptr<Sampler> CreateSampler(const Sampler::Config& config) {
        return std::make_unique<VulkanSampler>(m_device, config);
      }

      Sampler* DefaultNearestSampler() override {
        if (!m_default_nearest_sampler) {
          m_default_nearest_sampler = CreateSampler(Sampler::Config{
            .mag_filter = Sampler::FilterMode::Nearest,
            .min_filter = Sampler::FilterMode::Nearest,
            .mip_filter = Sampler::FilterMode::Nearest
          });
        }

        return m_default_nearest_sampler.get();
      }

      Sampler* DefaultLinearSampler() override {
        if (!m_default_linear_sampler) {
          m_default_linear_sampler = CreateSampler(Sampler::Config{
            .mag_filter = Sampler::FilterMode::Linear,
            .min_filter = Sampler::FilterMode::Linear,
            .mip_filter = Sampler::FilterMode::Linear
          });
        }

        return m_default_linear_sampler.get();
      }

      std::unique_ptr<RenderTarget> CreateRenderTarget(
        std::span<Texture::View* const> color_attachments,
        Texture::View* depth_stencil_attachment = nullptr
      ) override {
        return std::make_unique<VulkanRenderTarget>(m_device, color_attachments, depth_stencil_attachment);
      }

      std::unique_ptr<RenderPassBuilder> CreateRenderPassBuilder() override {
        return std::make_unique<VulkanRenderPassBuilder>(m_device);
      }

      auto CreateBindGroupLayout(std::span<const BindGroupLayout::Entry> entries) -> std::shared_ptr<BindGroupLayout> override {
        return std::make_shared<VulkanBindGroupLayout>(m_device, m_descriptor_pool, entries);
      }

      std::unique_ptr<BindGroup> CreateBindGroup(std::shared_ptr<BindGroupLayout> layout) override {
        return std::make_unique<VulkanBindGroup>(m_device, m_descriptor_pool, std::move(layout));
      }

      std::shared_ptr<PipelineLayout> CreatePipelineLayout(std::span<BindGroupLayout* const> bind_group_layouts) override {
        return std::make_unique<VulkanPipelineLayout>(m_device, bind_group_layouts);
      }

      std::unique_ptr<GraphicsPipelineBuilder> CreateGraphicsPipelineBuilder() override {
        return std::make_unique<VulkanGraphicsPipelineBuilder>(m_device, m_default_pipeline_layout);
      }

      std::unique_ptr<ComputePipeline> CreateComputePipeline(
        ShaderModule* shader_module,
        std::shared_ptr<PipelineLayout> layout
      ) override {
        return VulkanComputePipeline::Create(m_device, shader_module, layout);
      }

      std::shared_ptr<CommandPool> CreateGraphicsCommandPool(CommandPool::Usage usage) override {
        return std::make_shared<VulkanCommandPool>(m_device, m_queue_family_graphics, usage);
      }

      std::shared_ptr<CommandPool> CreateComputeCommandPool(CommandPool::Usage usage) override {
        return std::make_shared<VulkanCommandPool>(m_device, m_queue_family_compute, usage);
      }

      std::unique_ptr<CommandBuffer> CreateCommandBuffer(std::shared_ptr<CommandPool> pool) override {
        return std::make_unique<VulkanCommandBuffer>(m_device, std::move(pool));
      }

      std::unique_ptr<Fence> CreateFence(Fence::CreateSignalled create_signalled) override {
        return std::make_unique<VulkanFence>(m_device, create_signalled);
      }

      Queue* GraphicsQueue() override {
        return m_graphics_queue.get();
      }

      Queue* ComputeQueue() override {
        return m_compute_queue.get();
      }

      void WaitIdle() override {
        vkDeviceWaitIdle(m_device);
      }

    private:
      void CreateVmaAllocator() {
        VmaAllocatorCreateInfo info{
          .flags = 0,
          .physicalDevice = m_physical_device,
          .device = m_device,
          .preferredLargeHeapBlockSize = 0,
          .pAllocationCallbacks = nullptr,
          .pDeviceMemoryCallbacks = nullptr,
          .pHeapSizeLimit = nullptr,
          .pVulkanFunctions = nullptr,
          .instance = m_instance,
          .vulkanApiVersion = VK_API_VERSION_1_2,
          .pTypeExternalMemoryHandleTypes = nullptr
        };

        if (vmaCreateAllocator(&info, &m_allocator) != VK_SUCCESS) {
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

        if (vkCreateDescriptorPool(m_device, &info, nullptr, &m_descriptor_pool) != VK_SUCCESS) {
          ZEPHYR_PANIC("VulkanRenderDevice: failed to create descriptor pool");
        }
      }

      void CreateQueues() {
        m_graphics_queue = CreateQueueForFamily(m_queue_family_graphics);

        if(m_queue_family_graphics != m_queue_family_compute) {
          m_compute_queue = CreateQueueForFamily(m_queue_family_compute);
        } else {
          m_compute_queue = m_graphics_queue;
        }
      }

      std::unique_ptr<VulkanQueue> CreateQueueForFamily(u32 queue_family_index) {
        VkQueue queue;

        vkGetDeviceQueue(m_device, queue_family_index, 0, &queue);

        return std::make_unique<VulkanQueue>(queue);
      }

      VkInstance m_instance;
      VkPhysicalDevice m_physical_device;
      VkDevice m_device;
      VkDescriptorPool m_descriptor_pool;
      VmaAllocator m_allocator;
      std::shared_ptr<VulkanQueue> m_graphics_queue;
      std::shared_ptr<VulkanQueue> m_compute_queue;
      u32 m_queue_family_graphics;
      u32 m_queue_family_compute;

      std::shared_ptr<PipelineLayout> m_default_pipeline_layout;
      std::unique_ptr<Sampler> m_default_nearest_sampler;
      std::unique_ptr<Sampler> m_default_linear_sampler;
  };

  std::unique_ptr<RenderDevice> CreateVulkanRenderDevice(const VulkanRenderDeviceOptions& options) {
    return std::make_unique<VulkanRenderDevice>(options);
  }

} // namespace zephyr
