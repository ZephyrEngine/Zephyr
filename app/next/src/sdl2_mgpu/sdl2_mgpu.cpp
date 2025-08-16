
#include "zephyr/panic.hpp"
#include <SDL_syswm.h>

#include "sdl2_mgpu.hpp"

#ifdef SDL_VIDEO_DRIVER_COCOA
  extern "C" CAMetalLayer* metal_layer_from_ns_window(NSWindow* ns_window);
#endif

namespace zephyr {

Result<MGPUResult, MGPUSurface> mgpu_surface_from_sdl_window(MGPUInstance mgpu_instance, SDL_Window* sdl_window) {
  MGPUSurface mgpu_surface{};

  SDL_SysWMinfo wm_info{};
  SDL_VERSION(&wm_info.version);
  if(!SDL_GetWindowWMInfo(sdl_window, &wm_info)) {
    ZEPHYR_PANIC("SDL_GetWindowWMInfo failed()");
  }

  MGPUSurfaceCreateInfo surface_create_info{};

#if defined(SDL_VIDEO_DRIVER_WINDOWS)
  surface_create_info.win32 = {
    .hinstance = wm_info.info.win.hinstance,
    .hwnd = wm_info.info.win.window
  };
#elif defined(SDL_VIDEO_DRIVER_COCOA)
  surface_create_info.metal = {
    .metal_layer = metal_layer_from_ns_window(wm_info.info.cocoa.window)
  };
#elif defined(SDL_VIDEO_DRIVER_WAYLAND)  || defined(SDL_VIDEO_DRIVER_X11)
  switch(wm_info.subsystem) {
    case SDL_SYSWM_WAYLAND: {
      surface_create_info.wayland.display = wm_info.info.wl.display;
      surface_create_info.wayland.surface = wm_info.info.wl.surface;
      break;
    }
    case SDL_SYSWM_X11: {
      surface_create_info.x11.display = wm_info.info.x11.display;
      surface_create_info.x11.window = wm_info.info.x11.window;
      break;
    }
    default: {
      ATOM_PANIC("unexpected wm_info.subsystem: {}", wm_info.subsystem);
    }
  }
#else
  #error "Unsupported SDL video driver"
#endif

  MGPUResult mgpu_result = mgpuInstanceCreateSurface(mgpu_instance, &surface_create_info, &mgpu_surface);
  if(mgpu_result != MGPU_SUCCESS) {
    return mgpu_result;
  }
  return mgpu_surface;
}

} // namespace zephyr
