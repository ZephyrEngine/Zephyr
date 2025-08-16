
#pragma once

#include "zephyr/result.hpp"
#include "mgpu/mgpu.h"
#include <SDL.h>

// TODO: move this to the engine module
#define MGPU_CHECK(result_expression) \
  do { \
    MGPUResult result = result_expression; \
    if(result != MGPU_SUCCESS) \
      ZEPHYR_PANIC("MGPU error: {} ({})", "" # result_expression, mgpuResultCodeToString(result)); \
  } while(0)

namespace zephyr {

Result<MGPUResult, MGPUSurface> mgpu_surface_from_sdl_window(MGPUInstance mgpu_instance, SDL_Window* sdl_window);

} // namespace zephyr
