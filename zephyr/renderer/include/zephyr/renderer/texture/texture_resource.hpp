
#pragma once

#include <zephyr/gpu/sampler.hpp>
#include <zephyr/renderer/resource.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>

namespace zephyr {

//  class SamplerResource : public Resource, public NonCopyable, public NonMoveable {
//    public:
//  };
//
//  class Texture2DResource : public Resource, public NonCopyable, public NonMoveable {
//    public:
//
//  };

  class Texture2D : public Resource, public NonCopyable, public NonMoveable {
    public:
      enum class Format {
        RGBA
      };

      enum class DataType {
        UnsignedByte
      };

      enum class ColorSpace {
        LinearColorSpace,
        SRGBColorSpace
      };




    private:
  };

} // namespace zephyr
