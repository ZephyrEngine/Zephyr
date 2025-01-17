
#pragma once

#include <zephyr/renderer/resource/resource.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>
#include <zephyr/panic.hpp>
#include <memory>

namespace zephyr {

class TextureBase : public Resource {
  public:
    enum class Format {
      RGBA
    };

    enum class DataType {
      UnsignedByte
    };

    enum class ColorSpace {
      LinearSRGB,
      NonLinearSRGB
    };

    [[nodiscard]] Format GetFormat() const {
      return m_format;
    }

    [[nodiscard]] DataType GetDataType() const {
      return m_data_type;
    }

    [[nodiscard]] ColorSpace GetColorSpace() const {
      return m_color_space;
    }

    [[nodiscard]] size_t GetNumberOfComponents() const {
      switch(GetFormat()) {
        case Format::RGBA: return 4u;
        default: ZEPHYR_PANIC("Unimplemented texture format: {}", (int)GetFormat());
      }
    }

    [[nodiscard]] size_t GetComponentSize() const {
      switch(GetDataType()) {
        case DataType::UnsignedByte: return 1u;
        default: ZEPHYR_PANIC("Unimplemented texture data type: {}", (int)GetDataType());
      }
    }

    [[nodiscard]] size_t GetTexelSize() const {
      return GetNumberOfComponents() * GetComponentSize();
    }

    [[nodiscard]] virtual size_t Size() const = 0;
    [[nodiscard]] virtual const void* Data() const = 0;
    [[nodiscard]] virtual void* Data() = 0;

  protected:
    TextureBase(Format format, DataType data_type, ColorSpace color_space)
        : m_format{format}
        , m_data_type{data_type}
        , m_color_space{color_space} {
    }

  private:
    Format m_format{};
    DataType m_data_type{};
    ColorSpace m_color_space{};
};

} // namespace zephyr
