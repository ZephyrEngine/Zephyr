
#pragma once

#include <zephyr/renderer/texture/texture_resource.hpp>

namespace zephyr {

  class TextureCube final : public TextureResource {
    public:
      enum class Face {
        PositiveX = 0,
        NegativeX = 1,
        PositiveY = 2,
        NegativeY = 3,
        PositiveZ = 4,
        NegativeZ = 5
      };

      explicit TextureCube(
        uint face_size,
        Format format = Format::RGBA,
        DataType data_type = DataType::UnsignedByte,
        ColorSpace color_space = ColorSpace::Linear
      )   : TextureResource{format, data_type, color_space}, m_face_size{face_size} {
        m_size = face_size * face_size * GetTexelSize() * k_number_of_faces;
        m_data = new u8[m_size];
      }

     ~TextureCube() override {
        delete[] m_data;
      }

      [[nodiscard]] uint GetWidth() const {
        return m_face_size;
      }

      [[nodiscard]] uint GetHeight() const {
        return m_face_size;
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

      template<typename T>
      [[nodiscard]] const T* Data(Face face) const {
        return (const T*)&m_data[(size_t)face * m_face_size * m_face_size * GetTexelSize()];
      }

      template<typename T>
      [[nodiscard]] T* Data(Face face) {
        return (T*)&m_data[(size_t)face * m_face_size * m_face_size * GetTexelSize()];
      }

    private:
      static constexpr size_t k_number_of_faces = 6u;

      uint m_face_size{};
      u8* m_data{};
      size_t m_size{};
  };

} // namespace zephyr
