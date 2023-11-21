
#pragma once

#include <zephyr/gpu/render_pass.hpp>
#include <vector>

namespace zephyr {

  class RenderTarget {
    public:
      virtual ~RenderTarget() = default;

      virtual void* Handle() = 0;
      virtual u32 GetWidth() = 0;
      virtual u32 GetHeight() = 0;
  };

}; // namespace zephyr
