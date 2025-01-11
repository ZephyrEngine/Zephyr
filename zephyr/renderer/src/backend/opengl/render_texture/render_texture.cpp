
#include <zephyr/panic.hpp>

#include "render_texture.hpp"

namespace zephyr {

OpenGLRenderTexture::OpenGLRenderTexture(u32 width, u32 height)
    : m_width{width}
    , m_height{height} {
  glCreateTextures(GL_TEXTURE_2D, 1u, &m_gl_texture);
  glTextureStorage2D(m_gl_texture, 1u, GL_RGBA8, (GLsizei)width, (GLsizei)height);
}

OpenGLRenderTexture::~OpenGLRenderTexture() {
  glDeleteTextures(1u, &m_gl_texture);
}

void OpenGLRenderTexture::UpdateData(std::span<const u8> data) {
  if(data.size() != m_width * m_height * sizeof(u32)) {
    ZEPHYR_PANIC("texture upload data has wrong size: {}", data.size());
  }
  glTextureSubImage2D(m_gl_texture, 0, 0, 0, (GLsizei)m_width, (GLsizei)m_height, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
}

} // namespace zephyr
