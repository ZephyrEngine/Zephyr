
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>
#include <memory>

#include "render_pass.hpp"

namespace zephyr {

struct VulkanCommandBuffer final : CommandBuffer {
  VulkanCommandBuffer(VkDevice device, CommandPool* pool)
      : device(device), pool(pool) {
    auto info = VkCommandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = (VkCommandPool)pool->Handle(),
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(device, &info, &buffer) != VK_SUCCESS) {
      ZEPHYR_PANIC("Vulkan: failed to allocate command buffer");
    }
  }

 ~VulkanCommandBuffer() override {
    vkFreeCommandBuffers(device, (VkCommandPool)pool->Handle(), 1, &buffer);
  }

  auto Handle() -> void* override {
    return (void*)buffer;
  }

  void Begin(OneTimeSubmit one_time_submit) override {
    const auto begin_info = VkCommandBufferBeginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = (VkCommandBufferUsageFlags)one_time_submit,
      .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(buffer, &begin_info);
  }

  void End() override {
    vkEndCommandBuffer(buffer);
  }

  void SetViewport(int x, int y, int width, int height) override {
    const VkViewport viewport{
      .x = (float)x,
      .y = (float)y,
      .width  = (float)width,
      .height = (float)height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };

    vkCmdSetViewport(buffer, 0, 1, &viewport);
  }

  void SetScissor(int x, int y, int width, int height) override {
    const VkRect2D scissor{
      .offset = {
        .x = x,
        .y = y
      },
      .extent = {
        .width  = (u32)width,
        .height = (u32)height
      }
    };

    vkCmdSetScissor(buffer, 0, 1, &scissor);
  }

  void CopyBuffer(
    Buffer* src,
    Buffer* dst,
    u64 size,
    u64 src_offset = 0,
    u64 dst_offset = 0
  ) override {
    const VkBufferCopy copy = {
      .srcOffset = src_offset,
      .dstOffset = dst_offset,
      .size = size
    };

    vkCmdCopyBuffer(buffer, (VkBuffer)src->Handle(), (VkBuffer)dst->Handle(), 1, &copy);
  }

  void CopyBufferToTexture(
    Buffer* buffer,
    Texture* texture,
    Texture::Layout texture_layout,
    std::span<BufferTextureCopyRegion const> regions
  ) override {
    VkBufferImageCopy vk_regions[regions.size()];

    for(size_t i = 0; i < regions.size(); i++) {
      auto& region = regions[i];

      vk_regions[i] = {
        .bufferOffset = region.buffer.offset,
        .bufferRowLength = region.buffer.row_length,
        .bufferImageHeight = region.buffer.image_height,
        .imageSubresource = {
          .aspectMask = (VkImageAspectFlagBits)region.texture.layers.aspect,
          .mipLevel = region.texture.layers.mip_level,
          .baseArrayLayer = region.texture.layers.base_layer,
          .layerCount = region.texture.layers.layer_count,
        },
        .imageOffset = {
          .x = region.texture.offset_x,
          .y = region.texture.offset_y,
          .z = region.texture.offset_z
        },
        .imageExtent = {
          .width = (u32)region.texture.width,
          .height = (u32)region.texture.height,
          .depth = (u32)region.texture.depth
        }
      };
    }

    vkCmdCopyBufferToImage(
      this->buffer,
      (VkBuffer)buffer->Handle(),
      (VkImage)texture->Handle(),
      (VkImageLayout)texture_layout,
      (u32)regions.size(),
      vk_regions
    );
  }

  void PushConstants(
    PipelineLayout* pipeline_layout,
    u32 offset,
    u32 size,
    void const* data
  ) override {
    vkCmdPushConstants(buffer, (VkPipelineLayout)pipeline_layout->Handle(), VK_SHADER_STAGE_ALL, offset, size, data);
  }

  void BeginRenderPass(
    RenderTarget* render_target,
    RenderPass* render_pass
  ) override {
    auto vk_render_pass = (VulkanRenderPass*)render_pass;
    auto& clear_values = vk_render_pass->GetClearValues();

    auto info = VkRenderPassBeginInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = vk_render_pass->Handle(),
      .framebuffer = (VkFramebuffer)render_target->Handle(),
      .renderArea = VkRect2D{
        .offset = VkOffset2D{
          .x = 0,
          .y = 0
        },
        .extent = VkExtent2D{
          .width = render_target->GetWidth(),
          .height = render_target->GetHeight()
        }
      },
      .clearValueCount = (u32)clear_values.size(),
      .pClearValues = clear_values.data()
    };

    vkCmdBeginRenderPass(buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  void EndRenderPass() override {
    vkCmdEndRenderPass(buffer);
  }

  void BindGraphicsPipeline(GraphicsPipeline* pipeline) override {
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pipeline->Handle());
  }

  void BindComputePipeline(ComputePipeline* pipeline) override {
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)pipeline->Handle());
  }

  void BindGraphicsBindGroup(
    u32 set,
    PipelineLayout* pipeline_layout,
    BindGroup* bind_group
  ) override {
    auto vk_pipeline_layout = (VkPipelineLayout)pipeline_layout->Handle();
    auto vk_descriptor_set = (VkDescriptorSet)bind_group->Handle();

    vkCmdBindDescriptorSets(
      buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      vk_pipeline_layout,
      set,
      1, &vk_descriptor_set,
      0, nullptr
    );
  }

  void BindComputeBindGroup(
    u32 set,
    PipelineLayout* pipeline_layout,
    BindGroup* bind_group
  ) override {
    auto vk_pipeline_layout = (VkPipelineLayout)pipeline_layout->Handle();
    auto vk_descriptor_set = (VkDescriptorSet)bind_group->Handle();

    vkCmdBindDescriptorSets(
      buffer,
      VK_PIPELINE_BIND_POINT_COMPUTE,
      vk_pipeline_layout,
      set,
      1, &vk_descriptor_set,
      0, nullptr
    );
  }

  void BindVertexBuffers(
    std::span<Buffer* const> buffers,
    u32 first_binding = 0
  ) override {
    VkBuffer buffer_handles[32];

    const VkDeviceSize buffer_offsets[32] = { 0 };

    if (buffers.size() > 32) {
      ZEPHYR_PANIC("VulkanCommandBuffer: can't bind more than 32 vertex buffers at once");
    }

    for (int i = 0; i < buffers.size(); i++) {
      buffer_handles[i] = (VkBuffer)buffers[i]->Handle();
    }

    vkCmdBindVertexBuffers(buffer, first_binding, buffers.size(), buffer_handles, buffer_offsets);
  }

  void BindIndexBuffer(
    Buffer* buffer,
    IndexDataType data_type,
    size_t offset = 0
  ) override {
    vkCmdBindIndexBuffer(this->buffer, (VkBuffer)buffer->Handle(), offset, (VkIndexType)data_type);
  }

  void Draw(
    u32 vertex_count,
    u32 instance_count,
    u32 first_vertex,
    u32 first_instance
  ) override {
    vkCmdDraw(buffer, vertex_count, instance_count, first_vertex, first_instance);
  }

  void DrawIndexed(
    u32 index_count,
    u32 instance_count,
    u32 first_index,
    s32 vertex_offset,
    u32 first_instance
  ) override {
    vkCmdDrawIndexed(buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
  }

  void DispatchCompute(u32 group_count_x, u32 group_count_y, u32 group_count_z) override {
    vkCmdDispatch(buffer, group_count_x, group_count_y, group_count_z);
  }

  void PipelineBarrier(
    PipelineStage src_stage,
    PipelineStage dst_stage,
    std::span<MemoryBarrier const> memory_barriers
  ) override {
    const auto barrier_count = memory_barriers.size();

    VkImageMemoryBarrier vk_image_barriers[barrier_count];
    VkBufferMemoryBarrier vk_buffer_barriers[barrier_count];
    VkMemoryBarrier vk_memory_barriers[barrier_count];

    u32 image_barrier_count = 0;
    u32 buffer_barrier_count = 0;
    u32 memory_barrier_count = 0;

    for (auto& barrier : memory_barriers) {
      if (barrier.type == MemoryBarrier::Type::Texture) {
        auto& texture_info = barrier.texture_info;

        vk_image_barriers[image_barrier_count++] = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          .pNext = nullptr,
          .srcAccessMask = (VkAccessFlags)barrier.src_access_mask,
          .dstAccessMask = (VkAccessFlags)barrier.dst_access_mask,
          .oldLayout = (VkImageLayout)texture_info.src_layout,
          .newLayout = (VkImageLayout)texture_info.dst_layout,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = (VkImage)texture_info.texture->Handle(),
          .subresourceRange = {
            .aspectMask = (VkImageAspectFlags)texture_info.range.aspect,
            .baseMipLevel = texture_info.range.base_mip,
            .levelCount = texture_info.range.mip_count,
            .baseArrayLayer = texture_info.range.base_layer,
            .layerCount = texture_info.range.layer_count
          }
        };
      } else if (barrier.type == MemoryBarrier::Type::Buffer) {
        auto& buffer_info = barrier.buffer_info;

        vk_buffer_barriers[buffer_barrier_count++] = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
          .pNext = nullptr,
          .srcAccessMask = (VkAccessFlags)barrier.src_access_mask,
          .dstAccessMask = (VkAccessFlags)barrier.dst_access_mask,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .buffer = (VkBuffer)buffer_info.buffer->Handle(),
          .offset = buffer_info.offset,
          .size = buffer_info.size
        };
      } else {
        vk_memory_barriers[memory_barrier_count++] = {
          .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
          .pNext = nullptr,
          .srcAccessMask = (VkAccessFlags)barrier.src_access_mask,
          .dstAccessMask = (VkAccessFlags)barrier.dst_access_mask
        };
      }
    }

    vkCmdPipelineBarrier(
      buffer,
      (VkPipelineStageFlags)src_stage,
      (VkPipelineStageFlags)dst_stage, 0,
      memory_barrier_count, vk_memory_barriers,
      buffer_barrier_count, vk_buffer_barriers,
      image_barrier_count, vk_image_barriers
    );
  }

private:
  VkDevice device;
  VkCommandBuffer buffer;
  CommandPool* pool;
};

} // namespace zephyr
