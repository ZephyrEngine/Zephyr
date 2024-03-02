
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/renderer/buffer/buffer_resource_base.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/vector_n.hpp>
#include <unordered_map>

#include "resource_uploader.hpp"

namespace zephyr {

  class BufferCache : public NonCopyable {
    public:
      BufferCache(
        std::shared_ptr<RenderDevice> render_device,
        std::shared_ptr<ResourceUploader> resource_uploader
      );

      Buffer* GetDeviceBuffer(const BufferResourceBase* buffer_resource);

    private:
      struct Entry {
        u64 current_version;
        std::unique_ptr<Buffer> device_buffer;
      };

      void CopyBufferResourceToDeviceBuffer(const BufferResourceBase* buffer, Buffer* device_buffer);

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<ResourceUploader> m_resource_uploader;
      std::unordered_map<u64, Entry> m_cache;
  };

} // namespace zephyr
