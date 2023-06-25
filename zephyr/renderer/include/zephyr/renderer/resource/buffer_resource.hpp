
#pragma once

#include <zephyr/gpu/buffer.hpp>
#include <zephyr/renderer/resource/resource.hpp>

namespace zephyr {

  class BufferResource : public RendererResource {
    public:
      [[nodiscard]] virtual size_t Size() const = 0;
      [[nodiscard]] virtual const void* Data() const = 0;
      [[nodiscard]] virtual Buffer::Usage Usage() const = 0;
  };

} // namespace zephyr
