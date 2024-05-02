
#include <zephyr/renderer/backend/render_backend_ogl.hpp>
#include <zephyr/panic.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL_opengl.h>

#include <array>
#include <optional>
#include <vector>

namespace zephyr {

  class OpenGLRenderGeometry final : public RenderGeometry {
    public:
      static OpenGLRenderGeometry* Build(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) {
        GLuint gl_vao;
        glCreateVertexArrays(1u, &gl_vao);

        std::optional<GLuint> maybe_gl_ibo{};

        if(number_of_indices > 0) {
          // @todo: use GL_STATIC_DRAW when possible.
          GLuint gl_ibo;
          glCreateBuffers(1u, &gl_ibo);
          glNamedBufferData(gl_ibo, (GLsizeiptr)(sizeof(u32) * number_of_indices), nullptr, GL_DYNAMIC_DRAW);
          glVertexArrayElementBuffer(gl_vao, gl_ibo);
          maybe_gl_ibo = gl_ibo;
        }

        size_t vbo_stride = 0u;
        std::array<size_t, 32> vbo_attribute_offsets{};

        const auto PackAttribute = [&](RenderGeometryAttribute attribute, int number_of_components) {
          if(layout.HasAttribute(attribute)) {
            glEnableVertexArrayAttrib(gl_vao, (int)attribute);
            glVertexArrayAttribFormat(gl_vao, (int)attribute, number_of_components, GL_FLOAT, GL_FALSE, vbo_stride);
            glVertexArrayAttribBinding(gl_vao, (int)attribute, 0u);

            vbo_attribute_offsets[(int)attribute] = vbo_stride;
            vbo_stride += sizeof(f32) * number_of_components;
          }
        };

        PackAttribute(RenderGeometryAttribute::Position, 3);
        PackAttribute(RenderGeometryAttribute::Normal, 3);
        PackAttribute(RenderGeometryAttribute::UV, 2);
        PackAttribute(RenderGeometryAttribute::Color, 4);

        // @todo: use GL_STATIC_DRAW when possible.
        GLuint gl_vbo;
        glCreateBuffers(1u, &gl_vbo);
        glNamedBufferData(gl_vbo, (GLsizeiptr)(vbo_stride * number_of_vertices), nullptr, GL_DYNAMIC_DRAW);
        glVertexArrayVertexBuffer(gl_vao, 0u, gl_vbo, 0u, (GLsizei)vbo_stride);

        OpenGLRenderGeometry* render_geometry = new OpenGLRenderGeometry{};
        render_geometry->m_gl_vao = gl_vao;
        render_geometry->m_gl_ibo = maybe_gl_ibo;
        render_geometry->m_gl_vbo = gl_vbo;
        render_geometry->m_vbo_stride = vbo_stride;
        render_geometry->m_vbo_attribute_offsets = vbo_attribute_offsets;
        render_geometry->m_number_of_indices = number_of_indices;
        render_geometry->m_number_of_vertices = number_of_vertices;
        return render_geometry;
      }

      void UpdateIndices(std::span<const u8> data) {
        // @todo: validation
        glNamedBufferSubData(m_gl_ibo.value(), 0u, (GLsizeiptr)data.size_bytes(), data.data());
      }

      void UpdateVertices(std::span<const u8> data) {
        //@todo: validation
        glNamedBufferSubData(m_gl_vbo, 0u, (GLsizeiptr)data.size_bytes(), data.data());
      }

      void Draw() {
        glBindVertexArray(m_gl_vao);

        if(m_gl_ibo.has_value()) {
          glDrawElements(GL_TRIANGLES, (GLsizei)m_number_of_indices, GL_UNSIGNED_INT, nullptr);
        } else {
          glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_number_of_vertices);
        }
      }

    private:
      static_assert((int)RenderGeometryAttribute::Count <= 32);

      OpenGLRenderGeometry() = default;

      GLuint m_gl_vao{};
      std::optional<GLuint> m_gl_ibo{};
      GLuint m_gl_vbo{};
      size_t m_vbo_stride{};
      std::array<size_t, 32> m_vbo_attribute_offsets{};
      size_t m_number_of_indices{};
      size_t m_number_of_vertices{};
  };

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

        glEnable(GL_DEPTH_TEST);
      }

      void DestroyContext() override {
        glDeleteBuffers(1u, &m_gl_ubo);
        glDeleteVertexArrays(1u, &m_gl_vao);
        glDeleteProgram(m_gl_shader_program);

        SDL_GL_DeleteContext(m_gl_context);
      }

      RenderGeometry* CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) override {
        return OpenGLRenderGeometry::Build(layout, number_of_vertices, number_of_indices);
      }

      void UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) override {
        dynamic_cast<OpenGLRenderGeometry*>(render_geometry)->UpdateIndices(data);
      }

      void UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) override {
        dynamic_cast<OpenGLRenderGeometry*>(render_geometry)->UpdateVertices(data);
      }

      void DestroyRenderGeometry(RenderGeometry* geometry) override {
        delete geometry;
      }

      void Render(const Matrix4& projection, std::span<const RenderObject> render_objects) override {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_gl_shader_program);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_gl_ubo);
        glNamedBufferSubData(m_gl_ubo, 0, sizeof(Matrix4), &projection);

        for(const RenderObject& render_object : render_objects) {
          glNamedBufferSubData(m_gl_ubo, sizeof(Matrix4), sizeof(Matrix4), &render_object.local_to_world);
          dynamic_cast<OpenGLRenderGeometry*>(render_object.render_geometry)->Draw();
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

          layout(location = 0) in vec3 a_position;
          layout(location = 2) in vec2 a_uv;
          layout(location = 3) in vec3 a_color;

          out vec3 v_color;

          void main() {
            v_color = a_color;
            gl_Position = u_transform.projection * u_transform.local_to_world * vec4(a_position, 1.0);
          }
        )", GL_VERTEX_SHADER);

        GLuint frag_shader = CreateShader(R"(
          #version 450 core

          layout(location = 0) out vec4 f_frag_color;

          in vec3 v_color;

          void main() {
            f_frag_color = vec4(v_color, 1.0);
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
