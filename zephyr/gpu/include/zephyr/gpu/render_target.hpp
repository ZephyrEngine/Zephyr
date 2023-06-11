
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/gpu/render_pass.hpp>
#include <vector>

namespace zephyr {

struct RenderTarget {
  virtual ~RenderTarget() = default;

  virtual auto Handle() -> void* = 0;
  virtual auto GetWidth()  -> u32 = 0;
  virtual auto GetHeight() -> u32 = 0;
};

}; // namespace zephyr
