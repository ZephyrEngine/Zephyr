
#pragma once

#include <zephyr/renderer/resource/buffer_resource.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/panic.hpp>
#include <cstring>
#include <span>

namespace zephyr {

  class VertexBuffer final : public BufferResource, public NonCopyable {
    public:
      VertexBuffer(size_t stride, size_t number_of_vertices)
          : m_stride{stride}, m_number_of_vertices{number_of_vertices} {
        m_size = number_of_vertices * stride;
        m_data = new u8[m_size];
      }

      VertexBuffer(size_t stride, std::span<const u8> data) : m_stride{stride} {
        m_number_of_vertices = data.size() / stride;
        m_size = m_number_of_vertices * stride;
        m_data = new u8[m_size];
        std::memcpy(m_data, data.data(), m_size);
      }

     ~VertexBuffer() override {
        delete[] m_data;
      }

      [[nodiscard]] size_t Size() const override {
        return m_size;
      }

      [[nodiscard]] const void* Data() const override {
        return Data<void>();
      }

      template<typename T>
      [[nodiscard]] const T* Data() const {
        return (const T*)m_data;
      }

      template<typename T>
      [[nodiscard]] T* Data() {
        return (T*)m_data;
      }

      [[nodiscard]] Buffer::Usage Usage() const override {
        return Buffer::Usage::VertexBuffer;
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

        return *(const T*)&m_data[address];
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

        *(T*)&m_data[address] = value;
      }

    private:
      u8* m_data{};
      size_t m_size{};
      size_t m_stride{};
      size_t m_number_of_vertices{};
  };

} // namespace zephyr
