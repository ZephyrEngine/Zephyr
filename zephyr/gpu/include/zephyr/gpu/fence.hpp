
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <zephyr/integer.hpp>

namespace zephyr {

struct Fence {
  enum class CreateSignalled {
    No = 0,
    Yes = 1
  };

  virtual ~Fence() = default;

  virtual auto Handle() -> void* = 0;
  virtual void Reset() = 0;
  virtual void Wait(u64 timeout_ns = ~0ULL) = 0;
};

} // namespace zephyr
