
#pragma once

#include <zephyr/gpu/buffer.hpp>
#include <zephyr/renderer/resource/resource.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>

namespace zephyr {

  class BufferResource : public RendererResource {
    public:
      [[nodiscard]] virtual size_t Size() const = 0;
      [[nodiscard]] virtual const void* Data() const = 0;
      [[nodiscard]] virtual Buffer::Usage Usage() const = 0;
  };

  class BufferResourceImpl : public BufferResource, public NonCopyable {
    public:
      explicit BufferResourceImpl(Buffer::Usage usage, size_t size) : m_usage{usage}, m_size{size} {
        m_data = new u8[size];
      }

      explicit BufferResourceImpl(Buffer::Usage usage, std::span<const u8> data) : m_usage{usage} {
        m_data = new u8[data.size()];
        m_size = data.size();
        std::memcpy(m_data, data.data(), m_size);
      }

     ~BufferResourceImpl() override {
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
