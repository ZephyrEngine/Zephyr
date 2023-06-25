
#pragma once

#include <zephyr/renderer/buffer/buffer_resource.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/integer.hpp>
#include <cstring>
#include <span>

namespace zephyr {

  class UniformBuffer final : public BufferResource {
    public:
      explicit UniformBuffer(size_t size) : BufferResource{Buffer::Usage::UniformBuffer, size} {
      }

      explicit UniformBuffer(std::span<const u8> data) : BufferResource{Buffer::Usage::UniformBuffer, data} {
      }
  };

} // namespace zephyr
