
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/vector_n.hpp>

namespace zephyr {

  class ResourceUploader : public NonCopyable {
    public:
      ResourceUploader(
        std::shared_ptr<RenderDevice> render_device,
        std::shared_ptr<CommandPool> command_pool,
        uint frames_in_flight
      );

      void BeginFrame();
      void FinalizeFrame();
      CommandBuffer* GetCurrentCommandBuffer();
      Buffer* GetCurrentStagingBuffer();
      size_t AllocateStagingMemory(size_t size);

    private:
      void CreateCommandBuffers();
      void CreateStagingBuffers();

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<CommandPool> m_command_pool;
      Vector_N<std::unique_ptr<CommandBuffer>, 3> m_command_buffers;
      Vector_N<std::unique_ptr<Buffer>, 3> m_staging_buffers;
      size_t m_staging_buffer_free_offset{0u};

      uint m_frame{0};
      uint m_frames_in_flight;
      CommandBuffer* m_current_command_buffer{};
      Buffer* m_current_staging_buffer{};
  };

} // namespace zephyr
