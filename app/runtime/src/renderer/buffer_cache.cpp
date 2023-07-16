
#include <zephyr/panic.hpp>
#include <cstring>

#include "buffer_cache.hpp"

namespace zephyr {

  BufferCache::BufferCache(
    std::shared_ptr<RenderDevice> render_device,
    std::shared_ptr<ResourceUploader> resource_uploader
  )   : m_render_device{std::move(render_device)}, m_resource_uploader{std::move(resource_uploader)} {
  }

  Buffer* BufferCache::GetDeviceBuffer(const BufferResourceBase* buffer_resource) {
    const size_t size = buffer_resource->Size();

    Entry& entry = m_cache[buffer_resource];

    const bool have_device_buffer = (bool)entry.device_buffer;

    if(!have_device_buffer) {
      entry.device_buffer = m_render_device->CreateBuffer(
        Buffer::Usage::CopyDst | buffer_resource->Usage(), Buffer::Flags::None, size);
    }

    if(!have_device_buffer || buffer_resource->CurrentVersion() != entry.current_version) {
      CopyBufferResourceToDeviceBuffer(buffer_resource, entry.device_buffer.get());
      entry.current_version = buffer_resource->CurrentVersion();
    }

    return entry.device_buffer.get();
  }

  void BufferCache::CopyBufferResourceToDeviceBuffer(const BufferResourceBase* buffer_resource, Buffer* device_buffer) {
    const size_t size = buffer_resource->Size();
    const size_t staging_buffer_offset = m_resource_uploader->AllocateStagingMemory(size);

    Buffer* const current_staging_buffer = m_resource_uploader->GetCurrentStagingBuffer();

    u8* src_data = (u8*)buffer_resource->Data();
    u8* dst_data = (u8*)current_staging_buffer->Data() + staging_buffer_offset;
    std::memcpy(dst_data, src_data, size);

    m_resource_uploader->GetCurrentCommandBuffer()->CopyBuffer(current_staging_buffer, device_buffer, size, staging_buffer_offset, 0u);
  }

} // namespace zephyr
