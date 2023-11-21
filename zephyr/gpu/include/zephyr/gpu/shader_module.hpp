
#pragma once

namespace zephyr {

  class ShaderModule {
    public:
      virtual ~ShaderModule() = default;

      virtual void* Handle() = 0;
  };

} // namespace zephyr
