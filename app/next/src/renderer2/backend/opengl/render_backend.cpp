
#include <zephyr/renderer2/backend/render_backend_ogl.hpp>
#include <zephyr/panic.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL_opengl.h>

namespace zephyr {

  class OpenGLRenderBackend final : public RenderBackend {
    public:
      explicit OpenGLRenderBackend(SDL_Window* sdl2_window) : m_window{sdl2_window} {
      }

      void InitializeContext() override {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        m_gl_context = SDL_GL_CreateContext(m_window);
        if(m_gl_context == nullptr) {
          ZEPHYR_PANIC("Failed to create OpenGL context: {}", SDL_GetError());
        }
        glewInit();
      }

      void DestroyContext() override {
        SDL_GL_DeleteContext(m_gl_context);
      }

      void Render(const Matrix4& projection, std::span<const RenderObject> render_objects) override {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }

      void SwapBuffers() override {
        SDL_GL_SwapWindow(m_window);
      }

    private:
      SDL_Window* m_window;
      SDL_GLContext m_gl_context{};
  };

  std::unique_ptr<RenderBackend> CreateOpenGLRenderBackendForSDL2(SDL_Window* sdl2_window) {
    return std::make_unique<OpenGLRenderBackend>(sdl2_window);
  }

} // namespace zephyr
