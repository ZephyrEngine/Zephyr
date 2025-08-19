

#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/panic.hpp>
#include <mgpu/mgpu.h>
#include <memory>

#define MGPU_CHECK(result_expression) \
  do { \
    MGPUResult result = result_expression; \
    if(result != MGPU_SUCCESS) \
      ZEPHYR_PANIC("MGPU error: {} ({})", "" # result_expression, mgpuResultCodeToString(result)); \
  } while(0)

namespace zephyr {

std::unique_ptr<RenderBackend> CreateMGPURenderBackend(MGPUInstance mgpu_instance, MGPUSurface mgpu_surface);

} // namespace zephyr
