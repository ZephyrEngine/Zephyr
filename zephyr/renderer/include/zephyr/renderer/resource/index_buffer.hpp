
#pragma once

#include <zephyr/gpu/enums.hpp>
#include <zephyr/renderer/resource/buffer_resource.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/panic.hpp>
#include <cstring>
#include <span>

namespace zephyr {

  class IndexBuffer final : public BufferResourceImpl {
    public:
      IndexBuffer(IndexDataType data_type, size_t number_of_indices)
          : BufferResourceImpl{Buffer::Usage::IndexBuffer, number_of_indices * GetDataTypeSize(data_type)}
          , m_data_type{data_type}
          , m_number_of_indices{number_of_indices} {
      }

      IndexBuffer(IndexDataType data_type, std::span<const u8> data)
          : BufferResourceImpl{Buffer::Usage::IndexBuffer, data}
          , m_data_type{data_type} {
        const size_t data_size_type = GetDataTypeSize(data_type);

        if(Size() % data_size_type != 0u) {
          ZEPHYR_PANIC("Buffer size is not divisible by the size of the underlying data type");
        }

        m_number_of_indices = Size() / GetDataTypeSize(data_type);
      }

      [[nodiscard]] IndexDataType GetDataType() const {
        return m_data_type;
      }

      [[nodiscard]] size_t GetNumberOfIndices() const {
        return m_number_of_indices;
      }

    private:
      static size_t GetDataTypeSize(IndexDataType data_type) {
        switch(data_type) {
          case IndexDataType::UInt16: return sizeof(u16);
          case IndexDataType::UInt32: return sizeof(u32);
          default: ZEPHYR_PANIC("Unhandled index data type: {}", (int)data_type);
        }
      }

      IndexDataType m_data_type{};
      size_t m_number_of_indices{};
  };

} // namespace zephyr
