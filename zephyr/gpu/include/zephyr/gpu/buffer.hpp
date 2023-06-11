
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/integer.hpp>
#include <cstring>

namespace zephyr {

struct Buffer {
  // subset of VkBufferUsageFlagBits:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkBufferUsageFlagBits  
  enum class Usage : u32 {
    CopySrc = 0x00000001,
    CopyDst = 0x00000002,
    UniformBuffer = 0x00000010,
    StorageBuffer = 0x00000020,
    IndexBuffer = 0x00000040,
    VertexBuffer = 0x00000080
  };

  enum class Flags {
    None = 0,
    HostVisible  = 0x00000001,
    HostAccessRandom = 0x00000002
  };

  virtual ~Buffer() = default;

  virtual auto Handle() -> void* = 0;
  virtual void Map() = 0;
  virtual void Unmap() = 0;
  virtual auto Data() -> void* = 0;
  virtual auto Size() const -> size_t = 0;
  virtual void Flush() = 0;
  virtual void Flush(size_t offset, size_t size) = 0;

  template<typename T>
  void Update(T const* data, size_t count = 1, size_t index = 0) {
    auto offset = index * sizeof(T);
    auto size = count * sizeof(T);
    auto range_end = offset + size;

    Map();

    if (range_end <= Size()) {
      std::memcpy((u8*)Data() + offset, data, size);
    }

    Flush(offset, size);
  }
};

constexpr Buffer::Usage operator|(Buffer::Usage lhs, Buffer::Usage rhs) {
  return (Buffer::Usage)((u32)lhs | (u32)rhs);
}

constexpr u32 operator&(Buffer::Usage lhs, Buffer::Usage rhs) {
  return (u32)lhs & (u32)rhs;
}

constexpr Buffer::Flags operator|(Buffer::Flags lhs, Buffer::Flags rhs) {
  return (Buffer::Flags)((u32)lhs | (u32)rhs);
}

constexpr u32 operator&(Buffer::Flags lhs, Buffer::Flags rhs) {
  return (u32)lhs & (u32)rhs;
}


} // namespace zephyr
