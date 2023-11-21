
#pragma once

#include <span>
#include <zephyr/gpu/command_buffer.hpp>
#include <zephyr/gpu/fence.hpp>

namespace zephyr {

  class Queue {
    public:
      virtual ~Queue() = default;

      virtual void* Handle() = 0;
      virtual void Submit(std::span<CommandBuffer* const> buffers, Fence* fence = nullptr) = 0;
      virtual void WaitIdle() = 0;
  };

} // namespace zephyr
