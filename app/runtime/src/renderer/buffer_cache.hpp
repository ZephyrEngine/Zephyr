
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/renderer/buffer_resource.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/vector_n.hpp>
#include <unordered_map>

namespace zephyr {

  class BufferCache : public NonCopyable {
    public:
      BufferCache(
        std::shared_ptr<RenderDevice> render_device,
        std::shared_ptr<CommandPool> command_pool,
        uint frames_in_flight
      );

      void BeginFrame();
      void FinalizeFrame();
      Buffer* GetDeviceBuffer(const BufferResource* buffer_resource);
      CommandBuffer* GetCurrentCommandBuffer();

    private:
      struct Entry {
        u64 current_version;
        std::unique_ptr<Buffer> device_buffer;
      };

      void CreateCommandBuffers();
      void CreateStagingBuffers();
      size_t AllocateStagingMemory(size_t size);
      void CopyBufferResourceToDeviceBuffer(const BufferResource* buffer, Buffer* device_buffer);

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<CommandPool> m_command_pool;
      Vector_N<std::unique_ptr<CommandBuffer>, 3> m_command_buffers;
      Vector_N<std::unique_ptr<Buffer>, 3> m_staging_buffers;
      size_t m_staging_buffer_free_offset{0u};

      uint m_frame{0};
      uint m_frames_in_flight;
      CommandBuffer* m_current_command_buffer{};
      Buffer* m_current_staging_buffer{};

      std::unordered_map<const BufferResource*, Entry> m_cache;
  };

} // namespace zephyr
