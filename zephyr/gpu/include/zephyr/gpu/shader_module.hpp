
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

namespace zephyr {

struct ShaderModule {
  virtual ~ShaderModule() = default;

  virtual auto Handle() -> void* = 0;
};

} // namespace zephyr
