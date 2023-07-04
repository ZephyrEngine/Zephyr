
#pragma once

#include <zephyr/renderer/texture/texture_resource.hpp>

namespace zephyr {

  class Texture2D final : public TextureResource {
    public:
      Texture2D(
        uint width,
        uint height,
        Format format = Format::RGBA,
        DataType data_type = DataType::UnsignedByte,
        ColorSpace color_space = ColorSpace::Linear
      )   : TextureResource{format, data_type, color_space}, m_width{width}, m_height{height} {
        m_size = width * height * GetTexelSize();
        m_data = new u8[m_size];
      }

     ~Texture2D() override {
        delete[] m_data;
      }

      [[nodiscard]] uint GetWidth() const {
        return m_width;
      }

      [[nodiscard]] uint GetHeight() const {
        return m_height;
      }

      [[nodiscard]] size_t Size() const override {
        return m_size;
      }

      [[nodiscard]] const void* Data() const override {
        return (const void*)m_data;
      }

      [[nodiscard]] void* Data() override {
        return (void*)m_data;
      }

      template<typename T>
      [[nodiscard]] const T* Data() const {
        return (const T*)m_data;
      }

      template<typename T>
      [[nodiscard]] T* Data() {
        return (T*)m_data;
      }

    private:
      uint m_width{};
      uint m_height{};
      u8* m_data{};
      size_t m_size{};
  };

} // namespace zephyr
