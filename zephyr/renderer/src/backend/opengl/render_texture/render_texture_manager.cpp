
#include "render_texture.hpp"
#include "render_texture_manager.hpp"

namespace zephyr {

RenderTexture* OpenGLRenderTextureManager::CreateRenderTexture(u32 width, u32 height) {
  return new OpenGLRenderTexture{width, height};
}

void OpenGLRenderTextureManager::UpdateRenderTextureData(RenderTexture* render_texture, std::span<const u8> data) {
  dynamic_cast<OpenGLRenderTexture*>(render_texture)->UpdateData(data);
}

void OpenGLRenderTextureManager::DestroyRenderTexture(RenderTexture* render_texture) {
  delete render_texture;
}

} // namespace zephyr
