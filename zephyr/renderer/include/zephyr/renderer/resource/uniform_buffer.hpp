
#pragma once

#include <zephyr/renderer/resource/buffer_resource.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/integer.hpp>
#include <cstring>
#include <span>

namespace zephyr {

  class UniformBuffer final : public BufferResourceImpl {
    public:
      explicit UniformBuffer(size_t size) : BufferResourceImpl{Buffer::Usage::UniformBuffer, size} {
      }

      explicit UniformBuffer(std::span<const u8> data) : BufferResourceImpl{Buffer::Usage::UniformBuffer, data} {
      }
  };

} // namespace zephyr
