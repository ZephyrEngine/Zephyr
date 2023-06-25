
#pragma once

#include <zephyr/renderer/resource/buffer_resource.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/integer.hpp>
#include <cstring>
#include <span>

namespace zephyr {

  class UniformBuffer final : public BufferResource, public NonCopyable {
    public:
      explicit UniformBuffer(size_t size) : m_size{size} {
        m_data = new u8[size];
      }

      explicit UniformBuffer(std::span<const u8> data) {
        m_data = new u8[data.size()];
        m_size = data.size();
        std::memcpy(m_data, data.data(), m_size);
      }

     ~UniformBuffer() override {
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
        return Buffer::Usage::UniformBuffer;
      }

    private:
      u8* m_data{};
      size_t m_size{};
  };

} // namespace zephyr
