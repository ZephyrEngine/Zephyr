
#pragma once

#include <zephyr/gpu/pipeline_layout.hpp>

namespace zephyr {

// @todo: consider sharing code between different types of pipelines?
struct ComputePipeline {
  virtual ~ComputePipeline() = default;

  virtual auto Handle() -> void* = 0;
  virtual auto GetLayout() -> PipelineLayout* = 0;
};

} // namespace zephyr