
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

        CreateShaderProgram();
        glGenVertexArrays(1u, &m_gl_vao);

        glCreateBuffers(1u, &m_gl_ubo);
        glNamedBufferData(m_gl_ubo, sizeof(Matrix4) * 2, nullptr, GL_DYNAMIC_DRAW);
      }

      void DestroyContext() override {
        glDeleteBuffers(1u, &m_gl_ubo);
        glDeleteVertexArrays(1u, &m_gl_vao);
        glDeleteProgram(m_gl_shader_program);

        SDL_GL_DeleteContext(m_gl_context);
      }

      void Render(const Matrix4& projection, std::span<const RenderObject> render_objects) override {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(m_gl_vao);
        glUseProgram(m_gl_shader_program);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_gl_ubo);
        glNamedBufferSubData(m_gl_ubo, 0, sizeof(Matrix4), &projection);

        for(const RenderObject& render_object : render_objects) {
          glNamedBufferSubData(m_gl_ubo, sizeof(Matrix4), sizeof(Matrix4), &render_object.local_to_world);
          glDrawArrays(GL_TRIANGLES, 0, 3);
        }
      }

      void SwapBuffers() override {
        SDL_GL_SwapWindow(m_window);
      }

    private:
      void CreateShaderProgram() {
        GLuint vert_shader = CreateShader(R"(
          #version 450 core

          layout(binding = 0, std140) uniform Transform {
            mat4 projection;
            mat4 local_to_world;
          } u_transform;

          void main() {
            switch(gl_VertexID) {
              case 0: gl_Position = u_transform.projection * u_transform.local_to_world * vec4(-0.5,  0.5, 0.0, 1.0); break;
              case 1: gl_Position = u_transform.projection * u_transform.local_to_world * vec4( 0.5,  0.5, 0.0, 1.0); break;
              case 2: gl_Position = u_transform.projection * u_transform.local_to_world * vec4( 0.0, -0.5, 0.0, 1.0); break;
            }
          }
        )", GL_VERTEX_SHADER);

        GLuint frag_shader = CreateShader(R"(
          #version 450 core

          layout(location = 0) out vec4 f_frag_color;

          void main() {
            f_frag_color = vec4(1.0, 0.0, 0.0, 1.0);
          }
        )", GL_FRAGMENT_SHADER);

        m_gl_shader_program = CreateProgram({{vert_shader, frag_shader}});
        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
      }

      static GLuint CreateShader(const char* glsl_code, GLenum type) {
        GLuint gl_shader = glCreateShader(type);

        const char* glsl_code_array[] = {glsl_code};
        glShaderSource(gl_shader, 1u, glsl_code_array, nullptr);
        glCompileShader(gl_shader);

        GLint compile_succeeded;
        glGetShaderiv(gl_shader, GL_COMPILE_STATUS, &compile_succeeded);
        if(compile_succeeded == GL_FALSE) {
          // TODO(fleroviux): let the caller decide how to deal with the error.
          GLint info_log_length;
          glGetShaderiv(gl_shader, GL_INFO_LOG_LENGTH, &info_log_length);

          GLchar* info_log = new GLchar[info_log_length];
          glGetShaderInfoLog(gl_shader, info_log_length, &info_log_length, info_log);
          ZEPHYR_PANIC("OpenGL: failed to compile GLSL shader:\n{}", info_log);
          delete[] info_log;
        }

        return gl_shader;
      }

      static GLuint CreateProgram(std::span<const GLuint> shaders) {
        GLuint gl_program = glCreateProgram();

        for(auto shader_object : shaders) {
          glAttachShader(gl_program, shader_object);
        }
        glLinkProgram(gl_program);

        // TODO: handle program link error?
        return gl_program;
      }

      SDL_Window* m_window;
      SDL_GLContext m_gl_context{};
      GLuint m_gl_shader_program{};
      GLuint m_gl_vao{};
      GLuint m_gl_ubo{};
  };

  std::unique_ptr<RenderBackend> CreateOpenGLRenderBackendForSDL2(SDL_Window* sdl2_window) {
    return std::make_unique<OpenGLRenderBackend>(sdl2_window);
  }

} // namespace zephyr
