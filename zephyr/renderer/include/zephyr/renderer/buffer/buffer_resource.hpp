
#pragma once

#include <zephyr/renderer/buffer/buffer_resource_base.hpp>
#include <zephyr/integer.hpp>
#include <span>

namespace zephyr {

  class BufferResource : public BufferResourceBase {
    public:
      explicit BufferResource(Buffer::Usage usage, size_t size) : m_usage{usage}, m_size{size} {
        m_data = new u8[size];
      }

      explicit BufferResource(Buffer::Usage usage, std::span<const u8> data) : m_usage{usage} {
        m_data = new u8[data.size()];
        m_size = data.size();
        std::memcpy(m_data, data.data(), m_size);
      }

     ~BufferResource() override {
        delete[] m_data;
      }

      [[nodiscard]] size_t Size() const final {
        return m_size;
      }

      [[nodiscard]] const void* Data() const final {
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

      [[nodiscard]] Buffer::Usage Usage() const final {
        return m_usage;
      }

    private:
      Buffer::Usage m_usage;
      u8* m_data{};
      size_t m_size{};
  };


} // namespace zephyr
