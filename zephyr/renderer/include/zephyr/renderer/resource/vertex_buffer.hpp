
#pragma once

#include <zephyr/renderer/resource/buffer_resource.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/panic.hpp>
#include <cstring>
#include <span>

namespace zephyr {

  class VertexBuffer final : public BufferResourceImpl {
    public:
      VertexBuffer(size_t stride, size_t number_of_vertices)
          : BufferResourceImpl{Buffer::Usage::VertexBuffer, stride * number_of_vertices}
          , m_stride{stride}
          , m_number_of_vertices{number_of_vertices} {
      }

      VertexBuffer(size_t stride, std::span<const u8> data)
          : BufferResourceImpl{Buffer::Usage::VertexBuffer, data}
          , m_stride{stride} {
        if(data.size() % stride != 0u) {
          ZEPHYR_PANIC("Buffer size is not divisible by the vertex stride");
        }

        m_number_of_vertices = data.size() / stride;
      }

      [[nodiscard]] size_t GetNumberOfVertices() const {
        return m_number_of_vertices;
      }

      [[nodiscard]] size_t GetStride() const {
        return m_stride;
      }

      template<typename T>
      T Read(size_t id, size_t offset, size_t component = 0) const {
        const size_t address = id * m_stride + offset + component * sizeof(T);

#ifndef NDEBUG
        const size_t address_hi = address + sizeof(T);

        if(address_hi > m_size || address_hi < address) {
          ZEPHYR_PANIC("Bad out-of-bounds read!");
        }
#endif

        return *(const T*)(Data<u8>() + address);
      }

      template<typename T>
      void Write(size_t id, const T& value, size_t offset, size_t component = 0) {
        const size_t address = id * m_stride + offset + component * sizeof(T);

#ifndef NDEBUG
        const size_t address_hi = address + sizeof(T);

        if(address_hi > m_size || address_hi < address) {
          ZEPHYR_PANIC("Bad out-of-bounds write!");
        }
#endif

        *(T*)(Data<u8>() + address) = value;
      }

    private:
      size_t m_stride{};
      size_t m_number_of_vertices{};
  };

} // namespace zephyr
