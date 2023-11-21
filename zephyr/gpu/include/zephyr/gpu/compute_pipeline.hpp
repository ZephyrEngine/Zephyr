
#pragma once

#include <zephyr/gpu/pipeline_layout.hpp>

namespace zephyr {

  // @todo: consider sharing code between different types of pipelines?
  struct ComputePipeline {
    virtual ~ComputePipeline() = default;

    virtual void* Handle() = 0;
    virtual PipelineLayout* GetLayout() = 0;
  };

} // namespace zephyr