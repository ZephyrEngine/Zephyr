
#pragma once

#include <zephyr/gpu/enums.hpp>
#include <zephyr/renderer/resource.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/panic.hpp>
#include <cstring>
#include <span>

namespace zephyr {

  class IndexBuffer : public RendererResource, public NonCopyable {
    public:
      IndexBuffer(IndexDataType data_type, size_t number_of_indices)
          : m_data_type{data_type}, m_number_of_indices{number_of_indices} {
        switch(data_type) {
          case IndexDataType::UInt16: m_size = number_of_indices * sizeof(u16); break;
          case IndexDataType::UInt32: m_size = number_of_indices * sizeof(u32); break;
          default: ZEPHYR_PANIC("Unhandled index data type: {}", (int)data_type);
        }

        m_data = new u8[m_size];
      }

      IndexBuffer(IndexDataType data_type, std::span<u8> data) : m_data_type{data_type} {
        switch(data_type) {
          case IndexDataType::UInt16: {
            m_number_of_indices = data.size() / sizeof(u16);
            m_size = m_number_of_indices * sizeof(u16);
            break;
          }
          case IndexDataType::UInt32: {
            m_number_of_indices = data.size() / sizeof(u32);
            m_size = m_number_of_indices * sizeof(u32);
            break;
          }
          default: ZEPHYR_PANIC("Unhandled index data type: {}", (int)data_type);
        }

        m_data = new u8[m_size];
        std::memcpy(m_data, data.data(), m_size);
      }

     ~IndexBuffer() {
        delete[] m_data;
      }

      [[nodiscard]] size_t Size() const {
        return m_size;
      }

      template<typename T = u8>
      [[nodiscard]] const T* Data() const {
        return (const T*)m_data;
      }

      template<typename T = u8>
      [[nodiscard]] T* Data() {
        return (T*)m_data;
      }

      [[nodiscard]] IndexDataType GetDataType() const {
        return m_data_type;
      }

      [[nodiscard]] size_t GetNumberOfIndices() const {
        return m_number_of_indices;
      }

    private:
      IndexDataType m_data_type{};
      u8* m_data{};
      size_t m_size{};
      size_t m_number_of_indices{};
  };

} // namespace zephyr
