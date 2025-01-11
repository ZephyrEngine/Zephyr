
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/integer.hpp>
#include <span>

namespace zephyr {

class OpenGLRenderTextureManager {
  public:
    RenderTexture* CreateRenderTexture(u32 width, u32 height);
    void UpdateRenderTextureData(RenderTexture* render_texture, std::span<const u8> data);
    void DestroyRenderTexture(RenderTexture* render_texture);
};

} // namespace zephyr
