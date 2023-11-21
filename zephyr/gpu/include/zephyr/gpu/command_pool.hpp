
#pragma once

#include <zephyr/integer.hpp>

namespace zephyr {

  class CommandPool {
    public:
      // subset of VkCommandPoolCreateFlagBits:
      // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkCommandPoolCreateFlagBits
      enum class Usage : u32 {
        Transient = 1,
        ResetCommandBuffer = 2
      };

      virtual ~CommandPool() = default;

      virtual void* Handle() = 0;
  };

  constexpr CommandPool::Usage operator|(CommandPool::Usage lhs, CommandPool::Usage rhs) {
    return (CommandPool::Usage)((u32)lhs | (u32)rhs);
  }

} // namespace zephyr
