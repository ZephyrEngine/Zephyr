
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <memory>
#include <SDL.h>

namespace zephyr {

std::unique_ptr<RenderBackend> CreateOpenGLRenderBackendForSDL2(SDL_Window* sdl2_window);

} // namespace zephyr
