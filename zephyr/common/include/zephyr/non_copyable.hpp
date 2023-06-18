
#pragma once

namespace zephyr {

  class NonCopyable {
    public:
      NonCopyable() = default;
      NonCopyable(const NonCopyable&) = delete;

      NonCopyable& operator=(const NonCopyable&) = delete;
  };

} // namespace zephyr
