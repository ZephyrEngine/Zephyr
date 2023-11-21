
#pragma once

namespace zephyr {

  class PipelineLayout {
    public:
      virtual ~PipelineLayout() = default;

      virtual void* Handle() = 0;
  };

} // namespace zephyr
