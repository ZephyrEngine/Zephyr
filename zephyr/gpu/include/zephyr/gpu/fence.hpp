
#pragma once

#include <zephyr/integer.hpp>

namespace zephyr {

  class Fence {
    public:
      enum class CreateSignalled {
        No = 0,
        Yes = 1
      };

      virtual ~Fence() = default;

      virtual void* Handle() = 0;
      virtual void Reset() = 0;
      virtual void Wait(u64 timeout_ns = ~0ULL) = 0;
  };

} // namespace zephyr
