
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <span>
#include <zephyr/gpu/command_buffer.hpp>
#include <zephyr/gpu/fence.hpp>

namespace zephyr {

struct Queue {
  virtual ~Queue() = default;

  virtual auto Handle() -> void* = 0;
  virtual void Submit(std::span<CommandBuffer* const> buffers, Fence* fence = nullptr) = 0;
  virtual void WaitIdle() = 0;
};

} // namespace zephyr
