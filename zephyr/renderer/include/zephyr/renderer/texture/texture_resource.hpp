
#pragma once

#include <zephyr/renderer/texture/sampler_resource.hpp>
#include <zephyr/renderer/resource.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>
#include <zephyr/panic.hpp>
#include <memory>

namespace zephyr {

  class TextureResource : public Resource {
    public:
      enum class Format {
        RGBA,
        Limit
      };

      enum class DataType {
        UnsignedByte,
        Limit
      };

      enum class ColorSpace {
        Linear,
        SRGB,
        Limit
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

      [[nodiscard]] const std::shared_ptr<SamplerResource>& GetSampler() const {
        return m_sampler;
      }

      void SetSampler(std::shared_ptr<SamplerResource> sampler) {
        m_sampler = std::move(sampler);
      }

    protected:
      TextureResource(Format format, DataType data_type, ColorSpace color_space)
          : m_format{format}, m_data_type{data_type}, m_color_space{color_space} {
      }

    private:
      Format m_format{};
      DataType m_data_type{};
      ColorSpace m_color_space{};
      std::shared_ptr<SamplerResource> m_sampler;
  };

} // namespace zephyr
