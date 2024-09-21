
#pragma once

#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>

namespace zephyr {

struct Component : NonCopyable, NonMoveable {
  virtual ~Component() = default;
};

} // namespace zephyr
