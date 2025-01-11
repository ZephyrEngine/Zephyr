
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <span>
#include <GL/glew.h>
#include <GL/gl.h>

namespace zephyr {

class OpenGLRenderTexture final : public RenderTexture {
  public:
    OpenGLRenderTexture(u32 width, u32 height);
   ~OpenGLRenderTexture() override;

    void UpdateData(std::span<const u8> data);

  private:
    GLuint m_gl_texture{};
    u32 m_width;
    u32 m_height;
};

} // namespace zephyr
