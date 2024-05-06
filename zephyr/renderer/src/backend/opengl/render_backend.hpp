
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/panic.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "render_geometry.hpp"

namespace zephyr {

  class OpenGLRenderBackend final : public RenderBackend {
    public:
      explicit OpenGLRenderBackend(SDL_Window* sdl2_window);

      void InitializeContext() override;
      void DestroyContext() override;

      RenderGeometry* CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) override;
      void UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) override;
      void UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) override;
      void DestroyRenderGeometry(RenderGeometry* render_geometry) override;

      void Render(const Matrix4& view_projection, std::span<const RenderObject> render_objects) override;

      void SwapBuffers() override;

    private:
      void CreateShaderProgram();

      static GLuint CreateShader(const char* glsl_code, GLenum type);
      static GLuint CreateProgram(std::span<const GLuint> shaders);

      SDL_Window* m_window;
      SDL_GLContext m_gl_context{};
      GLuint m_gl_shader_program{};
      GLuint m_gl_vao{};
      GLuint m_gl_ubo{};
      std::unique_ptr<OpenGLRenderGeometryManager> m_render_geometry_manager{};
  };

  std::unique_ptr<RenderBackend> CreateOpenGLRenderBackendForSDL2(SDL_Window* sdl2_window) {
    return std::make_unique<OpenGLRenderBackend>(sdl2_window);
  }

} // namespace zephyr
