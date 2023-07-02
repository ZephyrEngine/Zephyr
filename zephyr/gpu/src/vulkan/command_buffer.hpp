
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/backend/vulkan.hpp>
#include <zephyr/panic.hpp>
#include <memory>

#include "render_pass.hpp"

namespace zephyr {

struct VulkanCommandBuffer final : CommandBuffer {
  VulkanCommandBuffer(VkDevice device, std::shared_ptr<CommandPool> pool)
      : device(device), pool(std::move(pool)) {
    auto info = VkCommandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = (VkCommandPool)this->pool->Handle(),
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
    u32 buffer_offset,
    Texture* texture,
    Texture::Layout texture_layout,
    u32 texture_mip_level
  ) override {
    const VkBufferImageCopy region = {
      .bufferOffset = buffer_offset,
      .bufferRowLength = 0u,
      .bufferImageHeight = 0u,
      .imageSubresource = {
        .aspectMask = (VkImageAspectFlagBits)texture->DefaultSubresourceRange().aspect,
        .mipLevel = texture_mip_level,
        .baseArrayLayer = 0,
        .layerCount = texture->GetLayerCount()
      },
      .imageOffset = { .x = 0u, .y = 0u, .z = 0u },
      .imageExtent = {
        .width = texture->GetWidth(),
        .height = texture->GetHeight(),
        .depth = texture->GetDepth()
      }
    };

    vkCmdCopyBufferToImage(this->buffer, (VkBuffer)buffer->Handle(), (VkImage)texture->Handle(), (VkImageLayout)texture_layout, 1, &region);
  }

  void BlitTexture2D(
    Texture* src_texture,
    Texture* dst_texture,
    const Rect2D& src_rect,
    const Rect2D& dst_rect,
    Texture::Layout src_layout,
    Texture::Layout dst_layout,
    u32 src_mip_level,
    u32 dst_mip_level,
    Sampler::FilterMode filter
  ) override {
    const VkImageBlit region = {
      .srcSubresource = {
        .aspectMask = (VkImageAspectFlagBits)src_texture->DefaultSubresourceRange().aspect,
        .mipLevel = src_mip_level,
        .baseArrayLayer = 0u,
        .layerCount = src_texture->GetLayerCount()
      },
      .srcOffsets = {
        {
          .x = (int32_t)src_rect.x,
          .y = (int32_t)src_rect.y,
          .z = 0
        },
        {
          .x = (int32_t)(src_rect.x + src_rect.width),
          .y = (int32_t)(src_rect.y + src_rect.height),
          .z = 1
        }
      },
      .dstSubresource = {
        .aspectMask = (VkImageAspectFlagBits)dst_texture->DefaultSubresourceRange().aspect,
        .mipLevel = dst_mip_level,
        .baseArrayLayer = 0u,
        .layerCount = dst_texture->GetLayerCount()
      },
      .dstOffsets = {
        {
          .x = (int32_t)dst_rect.x,
          .y = (int32_t)dst_rect.y,
          .z = 0
        },
        {
          .x = (int32_t)(dst_rect.x + dst_rect.width),
          .y = (int32_t)(dst_rect.y + dst_rect.height),
          .z = 1
        }
      }
    };

    vkCmdBlitImage(buffer, (VkImage)src_texture->Handle(), (VkImageLayout)src_layout, (VkImage)dst_texture->Handle(), (VkImageLayout)dst_layout, 1, &region, (VkFilter)filter);
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

  void BindPipeline(GraphicsPipeline* pipeline) override {
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pipeline->Handle());
  }

  void BindPipeline(ComputePipeline* pipeline) override {
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)pipeline->Handle());
  }

  void BindBindGroup(
    PipelineBindPoint pipeline_bind_point,
    PipelineLayout* pipeline_layout,
    u32 set,
    BindGroup* bind_group
  ) override {
    const auto vk_pipeline_layout = (VkPipelineLayout)pipeline_layout->Handle();
    const auto vk_descriptor_set = (VkDescriptorSet)bind_group->Handle();

    vkCmdBindDescriptorSets(
      buffer,
      (VkPipelineBindPoint)pipeline_bind_point,
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
  
  void Barrier(
    Texture* texture,
    PipelineStage src_stage,
    PipelineStage dst_stage,
    Access src_access,
    Access dst_access,
    Texture::Layout src_layout,
    Texture::Layout dst_layout,
    u32 mip_level,
    u32 mip_count
  ) override {
    const VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = (VkAccessFlags)src_access,
      .dstAccessMask = (VkAccessFlags)dst_access,
      .oldLayout = (VkImageLayout)src_layout,
      .newLayout = (VkImageLayout)dst_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = (VkImage)texture->Handle(),
      .subresourceRange = {
        .aspectMask = (VkImageAspectFlagBits)texture->DefaultSubresourceRange().aspect,
        .baseMipLevel = mip_level,
        .levelCount = mip_count,
        .baseArrayLayer = 0u,
        .layerCount = texture->GetLayerCount()
      }
    };

    vkCmdPipelineBarrier(buffer, (VkPipelineStageFlags)src_stage, (VkPipelineStageFlags)dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  }

private:
  VkDevice device;
  VkCommandBuffer buffer;
  std::shared_ptr<CommandPool> pool;
};

} // namespace zephyr
