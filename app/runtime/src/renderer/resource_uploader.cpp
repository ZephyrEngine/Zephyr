
#include <zephyr/literal.hpp>
#include <zephyr/panic.hpp>

#include "resource_uploader.hpp"

namespace zephyr {

  ResourceUploader::ResourceUploader(
    std::shared_ptr<RenderDevice> render_device,
    std::shared_ptr<CommandPool> command_pool,
    uint frames_in_flight
  )   : m_render_device{std::move(render_device)}, m_command_pool{std::move(command_pool)}, m_frames_in_flight{frames_in_flight} {
    CreateCommandBuffers();
    CreateStagingBuffers();
  }

  void ResourceUploader::BeginFrame() {
    m_staging_buffer_free_offset = 0u;
    m_current_staging_buffer = m_staging_buffers[m_frame].get();
    m_current_command_buffer = m_command_buffers[m_frame].get();
    m_current_command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
    m_frame = (m_frame + 1u) % m_frames_in_flight;
  }

  void ResourceUploader::FinalizeFrame() {
    m_current_command_buffer->End();
  }

  CommandBuffer* ResourceUploader::GetCurrentCommandBuffer() {
    return m_current_command_buffer;
  }

  Buffer* ResourceUploader::GetCurrentStagingBuffer() {
    return m_current_staging_buffer;
  }

  void ResourceUploader::CreateCommandBuffers() {
    // @todo: create an utility for creating a set of command buffers?
    for(size_t i = 0; i < m_frames_in_flight; i++) {
      // @todo: the command buffer should take ownership of the command pool.
      m_command_buffers.PushBack(m_render_device->CreateCommandBuffer(m_command_pool.get()));
    }
  }

  void ResourceUploader::CreateStagingBuffers() {
    for(uint i = 0; i < m_frames_in_flight; i++) {
      m_staging_buffers.PushBack(m_render_device->CreateBuffer(
        Buffer::Usage::CopySrc, Buffer::Flags::HostVisible, 512_MiB));

      m_staging_buffers[i]->Map();
    }
  }

  size_t ResourceUploader::AllocateStagingMemory(size_t size) {
    // @todo: potentially align the size so that our buffer copies are always from aligned boundaries?
    const size_t old_free_offset = m_staging_buffer_free_offset;
    const size_t new_free_offset = old_free_offset + size;

    if(new_free_offset > m_current_staging_buffer->Size()) {
      ZEPHYR_PANIC("Ran out of staging buffer space");
    }

    if(new_free_offset < old_free_offset) {
      ZEPHYR_PANIC("Bad allocation size: {} bytes", size);
    }

    m_staging_buffer_free_offset = new_free_offset;

    return old_free_offset;
  }

} // namespace zephyr
