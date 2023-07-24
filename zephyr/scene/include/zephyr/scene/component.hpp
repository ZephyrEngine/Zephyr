
#pragma once

#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>

namespace zephyr {

  class Component : public NonCopyable, public NonMoveable {
    public:
      virtual ~Component() = default;
  };

} // namespace zephyr
