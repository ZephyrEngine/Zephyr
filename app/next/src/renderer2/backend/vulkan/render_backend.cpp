
#include <zephyr/renderer2/backend/render_backend_vk.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/float.hpp>
#include <zephyr/panic.hpp>
#include <vector>

#include "shader/triangle.frag.h"
#include "shader/triangle.vert.h"

namespace zephyr {

  class VulkanRenderBackend final : public RenderBackend {
    public:
      explicit VulkanRenderBackend(const VulkanRenderBackendProps& props)
          : m_vk_instance{props.vk_instance}
          , m_vk_surface{props.vk_surface} {
      }

      void InitializeContext() override {
        CreateLogicalDevice(true);
        CreateSwapChain(m_vk_present_queue_family_indices);
        CreateCommandPool(m_vk_graphics_compute_queue_family_index);
        CreateCommandBuffer();
        CreateSemaphore();
        CreateFence();
        CreateRenderPass();
        CreateFramebuffers();
        CreateGraphicsPipeline();

        PrepareNextFrame();
      }

      void DestroyContext() override {
        vkDeviceWaitIdle(m_vk_device);

        vkDestroyPipeline(m_vk_device, m_vk_pipeline, nullptr);
        vkDestroyPipelineLayout(m_vk_device, m_vk_pipeline_layout, nullptr);

        for(auto framebuffer : m_vk_swap_chain_fbs) {
          vkDestroyFramebuffer(m_vk_device, framebuffer, nullptr);
        }

        vkDestroyRenderPass(m_vk_device, m_vk_render_pass, nullptr);

        vkDestroySemaphore(m_vk_device, m_vk_semaphore, nullptr);
        vkDestroyFence(m_vk_device, m_vk_fence, nullptr);

        vkFreeCommandBuffers(m_vk_device, m_vk_command_pool, 1u, &m_vk_command_buffer);
        vkDestroyCommandPool(m_vk_device, m_vk_command_pool, nullptr);

        for(auto view : m_vk_swap_chain_views) {
          vkDestroyImageView(m_vk_device, view, nullptr);
        }
        vkDestroySwapchainKHR(m_vk_device, m_vk_swap_chain, nullptr);

        vkDestroyDevice(m_vk_device, nullptr);
      }

      void Render(const Matrix4& projection, std::span<const RenderObject> render_objects) override {
        const VkClearValue clear_value{
          .color = VkClearColorValue{
            .float32 = {0.01f, 0.01f, 0.01f, 1.0f}
          }
        };

        const VkRenderPassBeginInfo render_pass_begin_info{
          .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
          .pNext = nullptr,
          .renderPass = m_vk_render_pass,
          .framebuffer = m_vk_swap_chain_fbs[m_vk_swap_chain_image_index],
          .renderArea = VkRect2D{
            .offset = VkOffset2D{0u, 0u},
            .extent = VkExtent2D{1920u, 1080u}
          },
          .clearValueCount = 1u,
          .pClearValues = &clear_value
        };

        vkCmdBeginRenderPass(m_vk_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vk_pipeline);
        vkCmdPushConstants(m_vk_command_buffer, m_vk_pipeline_layout, VK_SHADER_STAGE_ALL, 0u, sizeof(Matrix4), &projection);

        for(const RenderObject& render_object : render_objects) {
          vkCmdPushConstants(m_vk_command_buffer, m_vk_pipeline_layout, VK_SHADER_STAGE_ALL, sizeof(Matrix4), sizeof(Matrix4), &render_object.local_to_world);
          vkCmdDraw(m_vk_command_buffer, 3u, 1u, 0u, 0u);
        }

        vkCmdEndRenderPass(m_vk_command_buffer);
      }

      void SwapBuffers() override {
        // Complete command recording and mark the command buffer as ready for queue submission.
        vkEndCommandBuffer(m_vk_command_buffer);

        // Execute our command buffer once the acquired image is ready and signal the command buffer fence once the command buffer has executed.
        const VkPipelineStageFlags waiting_stages = VK_PIPELINE_STAGE_TRANSFER_BIT; // This should be enough for now.
        const VkSubmitInfo submit_info{
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .pNext = nullptr,
          .waitSemaphoreCount = 1u,
          .pWaitSemaphores = &m_vk_semaphore,
          .pWaitDstStageMask = &waiting_stages,
          .commandBufferCount = 1u,
          .pCommandBuffers = &m_vk_command_buffer,
          .signalSemaphoreCount = 0u,
          .pSignalSemaphores = nullptr
        };
        if(vkQueueSubmit(m_vk_graphics_compute_queue, 1u, &submit_info, m_vk_fence) != VK_SUCCESS) {
          ZEPHYR_PANIC("Queue submit failed :c");
        }

        // TODO: do we need to wait for a semaphore?
        const VkPresentInfoKHR present_info{
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .pNext = nullptr,
          .waitSemaphoreCount = 0u,
          .pWaitSemaphores = nullptr,
          .swapchainCount = 1u,
          .pSwapchains = &m_vk_swap_chain,
          .pImageIndices = &m_vk_swap_chain_image_index,
          .pResults = nullptr
        };
        vkQueuePresentKHR(m_vk_graphics_compute_queue, &present_info);

        PrepareNextFrame();
      }

    private:
      void PrepareNextFrame() {
        if(vkAcquireNextImageKHR(m_vk_device, m_vk_swap_chain, 0ull, m_vk_semaphore, VK_NULL_HANDLE, &m_vk_swap_chain_image_index) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to acquire swap chain image");
        }

        // Wait until the command buffer has been processed by the GPU and can be used again.
        vkWaitForFences(m_vk_device, 1u, &m_vk_fence, VK_TRUE, ~0ull);
        vkResetFences(m_vk_device, 1u, &m_vk_fence);

        // Release any commands that were already recorded into the command buffer back to the command pool.
        vkResetCommandBuffer(m_vk_command_buffer, 0);

        // Begin recording commands into the command buffer
        const VkCommandBufferBeginInfo begin_cmd_buffer_info{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .pNext = nullptr,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          .pInheritanceInfo = nullptr
        };
        vkBeginCommandBuffer(m_vk_command_buffer, &begin_cmd_buffer_info);
      }

      // Initial setup mess below:

      VulkanPhysicalDevice* PickPhysicalDevice() {
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

      void CreateLogicalDevice(bool enable_validation_layers) {
        VulkanPhysicalDevice* vk_physical_device = PickPhysicalDevice();

        std::vector<const char*> required_device_layers{};

        if(enable_validation_layers && vk_physical_device->QueryDeviceLayerSupport("VK_LAYER_KHRONOS_validation")) {
          required_device_layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        std::vector<const char*> required_device_extensions{"VK_KHR_swapchain"};

        for(auto extension_name : required_device_extensions) {
          if(!vk_physical_device->QueryDeviceExtensionSupport(extension_name)) {
            ZEPHYR_PANIC("Could not find device extension: {}", extension_name);
          }
        }

        if(vk_physical_device->QueryDeviceExtensionSupport("VK_KHR_portability_subset")) {
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

          for(const auto& queue_family : vk_physical_device->EnumerateQueueFamilies()) {
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

            m_vk_graphics_compute_queue_family_index = graphics_plus_compute_queue_family_index.value();
            m_vk_present_queue_family_indices.push_back(m_vk_graphics_compute_queue_family_index);
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

            m_vk_present_queue_family_indices.push_back(dedicated_compute_queue_family_index.value());

            ZEPHYR_INFO("Got an asynchronous compute queue !");
          }
        }

        m_vk_device = vk_physical_device->CreateLogicalDevice(queue_create_infos, required_device_extensions);

        vkGetDeviceQueue(m_vk_device, graphics_plus_compute_queue_family_index.value(), 0u, &m_vk_graphics_compute_queue);

        if(dedicated_compute_queue_family_index.has_value()) {
          VkQueue queue;
          vkGetDeviceQueue(m_vk_device, dedicated_compute_queue_family_index.value(), 0u, &queue);
          m_vk_dedicated_compute_queue = queue;
        }
      }

      void CreateSwapChain(const std::vector<u32>& present_queue_family_indices) {
        // TODO: query for supported swap chain configurations

        const VkSwapchainCreateInfoKHR create_info{
          .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
          .pNext = nullptr,
          .flags = 0,
          .surface = m_vk_surface,
          .minImageCount = 2,
          .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
          .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
          .imageExtent = VkExtent2D{
            .width = 1920,
            .height = 1080
          },
          .imageArrayLayers = 1,
          .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, // TODO
          .imageSharingMode = VK_SHARING_MODE_CONCURRENT, // What are the implications of this?
          .queueFamilyIndexCount = (u32)present_queue_family_indices.size(),
          .pQueueFamilyIndices = present_queue_family_indices.data(),
          .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
          .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
          .presentMode = VK_PRESENT_MODE_FIFO_KHR,
          .clipped = VK_TRUE,
          .oldSwapchain = nullptr
        };

        if(vkCreateSwapchainKHR(m_vk_device, &create_info, nullptr, &m_vk_swap_chain) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create swap chain :c");
        }

        u32 image_count;
        vkGetSwapchainImagesKHR(m_vk_device, m_vk_swap_chain, &image_count, nullptr);
        m_vk_swap_chain_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_vk_device, m_vk_swap_chain, &image_count, m_vk_swap_chain_images.data());

        for(auto image : m_vk_swap_chain_images) {
          const VkImageViewCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .components = {
              .r = VK_COMPONENT_SWIZZLE_IDENTITY,
              .g = VK_COMPONENT_SWIZZLE_IDENTITY,
              .b = VK_COMPONENT_SWIZZLE_IDENTITY,
              .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0u,
              .levelCount = 1u,
              .baseArrayLayer = 0u,
              .layerCount = 1u
            }
          };

          VkImageView view;

          if(vkCreateImageView(m_vk_device, &create_info, nullptr, &view) != VK_SUCCESS) {
            ZEPHYR_PANIC("Failed to create image view");
          }

          m_vk_swap_chain_views.push_back(view);
        }

        ZEPHYR_INFO("Swap chain successfully created");
      }

      void CreateCommandPool(u32 queue_family_index) {
        const VkCommandPoolCreateInfo create_info{
          .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
          .pNext = nullptr,
          .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
          .queueFamilyIndex = queue_family_index
        };

        if(vkCreateCommandPool(m_vk_device, &create_info, nullptr, &m_vk_command_pool) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create command pool");
        }
      }

      void CreateCommandBuffer() {
        const VkCommandBufferAllocateInfo alloc_info{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .pNext = nullptr,
          .commandPool = m_vk_command_pool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1u
        };

        if(vkAllocateCommandBuffers(m_vk_device, &alloc_info, &m_vk_command_buffer) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to allocate command buffer");
        }
      }

      void CreateSemaphore() {
        const VkSemaphoreCreateInfo create_info{
          .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0
        };

        if(vkCreateSemaphore(m_vk_device, &create_info, nullptr, &m_vk_semaphore) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create semaphore");
        }
      }

      void CreateFence() {
        const VkFenceCreateInfo create_info{
          .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
          .pNext = nullptr,
          .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        if(vkCreateFence(m_vk_device, &create_info, nullptr, &m_vk_fence) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create fence");
        }
      }

      void CreateRenderPass() {
        const VkAttachmentDescription attachment_desc{
          .flags = 0,
          .format = VK_FORMAT_B8G8R8A8_SRGB,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        }; // TODO: check if a sub pass dependency is necessary (should be fine though)

        const VkAttachmentReference sub_pass_attachment_ref{
          .attachment = 0u,
          .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        const VkSubpassDescription sub_pass_desc{
          .flags = 0,
          .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
          .inputAttachmentCount = 0u,
          .pInputAttachments = nullptr,
          .colorAttachmentCount = 1u,
          .pColorAttachments = &sub_pass_attachment_ref,
          .pResolveAttachments = nullptr,
          .pDepthStencilAttachment = nullptr,
          .preserveAttachmentCount = 0u,
          .pPreserveAttachments = nullptr
        };

        const VkRenderPassCreateInfo create_info{
          .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .attachmentCount = 1u,
          .pAttachments = &attachment_desc,
          .subpassCount = 1u,
          .pSubpasses = &sub_pass_desc,
          .dependencyCount = 0u,
          .pDependencies = nullptr
        };

        if(vkCreateRenderPass(m_vk_device, &create_info, nullptr, &m_vk_render_pass) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create render pass");
        }
      }

      void CreateFramebuffers() {
        for(auto view : m_vk_swap_chain_views) {
          const VkFramebufferCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = m_vk_render_pass,
            .attachmentCount = 1u,
            .pAttachments = &view,
            .width = 1920u,
            .height = 1080u,
            .layers = 1u
          };

          VkFramebuffer framebuffer;

          if(vkCreateFramebuffer(m_vk_device, &create_info, nullptr, &framebuffer) != VK_SUCCESS) {
            ZEPHYR_PANIC("Failed to create framebuffer");
          }

          m_vk_swap_chain_fbs.push_back(framebuffer);
        }
      }

      void CreateGraphicsPipeline() {
        VkShaderModule vert_shader;
        VkShaderModule frag_shader;
        VkShaderModuleCreateInfo vert_create_info{
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .codeSize = sizeof(triangle_vert),
          .pCode = triangle_vert
        };
        VkShaderModuleCreateInfo frag_create_info{
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .codeSize = sizeof(triangle_frag),
          .pCode = triangle_frag
        };

        if(vkCreateShaderModule(m_vk_device, &vert_create_info, nullptr, &vert_shader) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create vertex shader");
        }
        if(vkCreateShaderModule(m_vk_device, &frag_create_info, nullptr, &frag_shader) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create fragment shader");
        }

        const VkPipelineShaderStageCreateInfo shader_stages[2] {
          {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr
          },
          {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr
          }
        };

        const VkPushConstantRange push_constant_range{
          .stageFlags = VK_SHADER_STAGE_ALL,
          .offset = 0u,
          .size = 128u
        };

        const VkPipelineLayoutCreateInfo layout_create_info{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .setLayoutCount = 0u,
          .pSetLayouts = nullptr,
          .pushConstantRangeCount = 1u,
          .pPushConstantRanges = &push_constant_range
        };
        if(vkCreatePipelineLayout(m_vk_device, &layout_create_info, nullptr, &m_vk_pipeline_layout) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create pipeline layout");
        }

        const VkPipelineVertexInputStateCreateInfo vertex_input_state{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .vertexBindingDescriptionCount = 0u,
          .pVertexBindingDescriptions = nullptr,
          .vertexAttributeDescriptionCount = 0u,
          .pVertexAttributeDescriptions = nullptr
        };

        const VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
          .primitiveRestartEnable = VK_FALSE
        };

        const VkViewport viewport{
          .x = 0,
          .y = 0,
          .width = 1920,
          .height = 1080,
          .minDepth = 0, // TODO
          .maxDepth = 1
        };
        const VkRect2D scissor{
          .offset = VkOffset2D{0, 0},
          .extent = VkExtent2D{0x7FFFFFFFul, 0x7FFFFFFFul}
        };
        const VkPipelineViewportStateCreateInfo viewport_state{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .viewportCount = 1u,
          .pViewports = &viewport,
          .scissorCount = 1u,
          .pScissors = &scissor
        };

        const VkPipelineRasterizationStateCreateInfo rasterization_state{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .depthClampEnable = VK_FALSE,
          .rasterizerDiscardEnable = VK_FALSE,
          .polygonMode = VK_POLYGON_MODE_FILL,
          .cullMode = VK_CULL_MODE_NONE,
          .frontFace = VK_FRONT_FACE_CLOCKWISE,
          .depthBiasEnable = VK_FALSE,
          .depthBiasConstantFactor = 0.0f,
          .depthBiasClamp = 0.0f,
          .depthBiasSlopeFactor = 0.0f,
          .lineWidth = 1.0f
        };

        const VkPipelineMultisampleStateCreateInfo multisample_state{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
          .sampleShadingEnable = VK_FALSE,
          .minSampleShading = 0.0f,
          .pSampleMask = nullptr,
          .alphaToCoverageEnable = VK_FALSE,
          .alphaToOneEnable = VK_FALSE
        };

        const VkPipelineColorBlendAttachmentState attachment_blend_state{
          .blendEnable = VK_FALSE,
          .colorWriteMask = 0xF
        };
        const VkPipelineColorBlendStateCreateInfo color_blend_state{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .logicOpEnable = VK_FALSE,
          .logicOp = VK_LOGIC_OP_NO_OP,
          .attachmentCount = 1u,
          .pAttachments = &attachment_blend_state,
          .blendConstants = {0, 0, 0, 0}
        };

        const VkGraphicsPipelineCreateInfo create_info{
          .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stageCount = 2u, // TODO
          .pStages = shader_stages, // TODO
          .pVertexInputState = &vertex_input_state,
          .pInputAssemblyState = &input_assembly_state,
          .pTessellationState = nullptr,
          .pViewportState = &viewport_state,
          .pRasterizationState = &rasterization_state,
          .pMultisampleState = &multisample_state,
          .pDepthStencilState = nullptr,
          .pColorBlendState = &color_blend_state,
          .pDynamicState = nullptr,
          .layout = m_vk_pipeline_layout,
          .renderPass = m_vk_render_pass,
          .subpass = 0u,
          .basePipelineHandle = VK_NULL_HANDLE,
          .basePipelineIndex = 0
        };

        if(vkCreateGraphicsPipelines(m_vk_device, VK_NULL_HANDLE, 1u, &create_info, nullptr, &m_vk_pipeline) != VK_SUCCESS) {
          ZEPHYR_PANIC("Failed to create graphics pipeline, welp")
        }
      }

      std::shared_ptr<VulkanInstance> m_vk_instance;
      VkDevice m_vk_device{};
      VkSurfaceKHR m_vk_surface;
      u32 m_vk_graphics_compute_queue_family_index{};
      std::vector<u32> m_vk_present_queue_family_indices{};
      VkQueue m_vk_graphics_compute_queue{};
      std::optional<VkQueue> m_vk_dedicated_compute_queue{};

      // Swap Chain
      VkSwapchainKHR m_vk_swap_chain{};
      std::vector<VkImage> m_vk_swap_chain_images{};
      std::vector<VkImageView> m_vk_swap_chain_views{};
      u32 m_vk_swap_chain_image_index{};

      // Command Buffer
      VkCommandPool m_vk_command_pool{};
      VkCommandBuffer m_vk_command_buffer{};

      // Synchronization
      VkSemaphore m_vk_semaphore{};
      VkFence m_vk_fence{};

      // Render Pass and Frame Buffers
      VkRenderPass m_vk_render_pass{};
      std::vector<VkFramebuffer> m_vk_swap_chain_fbs{};

      // Graphics Pipeline
      VkPipelineLayout m_vk_pipeline_layout{};
      VkPipeline m_vk_pipeline{};
  };

  std::unique_ptr<RenderBackend> CreateVulkanRenderBackend(const VulkanRenderBackendProps& props) {
    return std::make_unique<VulkanRenderBackend>(props);
  }

} // namespace zephyr
