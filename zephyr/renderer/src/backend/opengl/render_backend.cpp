
#include "render_backend.hpp"

namespace zephyr {

  OpenGLRenderBackend::OpenGLRenderBackend(SDL_Window* sdl2_window) : m_window{sdl2_window} {
  }

  void OpenGLRenderBackend::InitializeContext() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    m_gl_context = SDL_GL_CreateContext(m_window);
    if(m_gl_context == nullptr) {
      ZEPHYR_PANIC("Failed to create OpenGL context: {}", SDL_GetError());
    }
    glewInit();

    CreateDrawShaderProgram();
    CreateDrawListBuilderShaderProgram();

    glCreateBuffers(1u, &m_gl_draw_list_ssbo);
    glNamedBufferStorage(m_gl_draw_list_ssbo, sizeof(OpenGLDrawElementsIndirectCommand), nullptr, 0);

    glCreateBuffers(1u, &m_gl_ubo);
    glNamedBufferStorage(m_gl_ubo, sizeof(Matrix4) * 2, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glEnable(GL_DEPTH_TEST);

    m_render_geometry_manager = std::make_unique<OpenGLRenderGeometryManager>();
  }

  void OpenGLRenderBackend::DestroyContext() {
    m_render_geometry_manager.reset();

    glDeleteBuffers(1u, &m_gl_ubo);
    glDeleteBuffers(1u, &m_gl_draw_list_ssbo);
    glDeleteProgram(m_gl_draw_list_builder_program);
    glDeleteProgram(m_gl_draw_program);

    SDL_GL_DeleteContext(m_gl_context);
  }

  RenderGeometry* OpenGLRenderBackend::CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) {
    return m_render_geometry_manager->CreateRenderGeometry(layout, number_of_vertices, number_of_indices);
  }

  void OpenGLRenderBackend::UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) {
    m_render_geometry_manager->UpdateRenderGeometryIndices(render_geometry, data);
  }

  void OpenGLRenderBackend::UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) {
    m_render_geometry_manager->UpdateRenderGeometryVertices(render_geometry, data);
  }

  void OpenGLRenderBackend::DestroyRenderGeometry(RenderGeometry* render_geometry) {
    m_render_geometry_manager->DestroyRenderGeometry(render_geometry);
  }

  void OpenGLRenderBackend::Render(const Matrix4& view_projection, std::span<const RenderObject> render_objects) {
    glUseProgram(m_gl_draw_list_builder_program);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0u, m_render_geometry_manager->GetDrawCommandBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1u, m_gl_draw_list_ssbo);
    glDispatchCompute(1u, 1u, 1u);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0u, 0u);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1u, 0u);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4 test_transform = Matrix4::Translation(0, 0, -4);
    glUseProgram(m_gl_draw_program);
    glBindVertexArray(m_render_geometry_manager->GetVAOFromLayout(dynamic_cast<OpenGLRenderGeometry*>(render_objects[0].render_geometry)->GetLayout())); // TODO(fleroviux): fix this lo
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_gl_draw_list_ssbo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_gl_ubo);
    glNamedBufferSubData(m_gl_ubo, 0, sizeof(Matrix4), &view_projection);
    glNamedBufferSubData(m_gl_ubo, sizeof(Matrix4), sizeof(Matrix4), &test_transform); // TODO(fleroviux): remove me
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0u);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0u);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0u);

    /*glUseProgram(m_gl_draw_program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_gl_ubo);
    glNamedBufferSubData(m_gl_ubo, 0, sizeof(Matrix4), &view_projection);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_render_geometry_manager->GetDrawCommandBuffer());

    for(const RenderObject& render_object : render_objects) {
      glNamedBufferSubData(m_gl_ubo, sizeof(Matrix4), sizeof(Matrix4), &render_object.local_to_world);

      // TODO(fleroviux): avoid constant rebinding of VAO
      auto render_geometry = dynamic_cast<OpenGLRenderGeometry*>(render_object.render_geometry);
      auto indirect_buffer_offset = (const void*)(render_geometry->GetDrawCommandID() * sizeof(OpenGLDrawElementsIndirectCommand));
      glBindVertexArray(m_render_geometry_manager->GetVAOFromLayout(render_geometry->GetLayout()));
      if(render_geometry->GetNumberOfIndices() > 0) {
        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, indirect_buffer_offset);
      } else {
        glDrawArraysIndirect(GL_TRIANGLES, indirect_buffer_offset);
      }
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);*/
  }

  void OpenGLRenderBackend::SwapBuffers() {
    SDL_GL_SwapWindow(m_window);
  }

  void OpenGLRenderBackend::CreateDrawShaderProgram() {
    GLuint vert_shader = CreateShader(R"(
      #version 450 core

      layout(binding = 0, std140) uniform Transform {
        mat4 projection;
        mat4 local_to_world;
      } u_transform;

      layout(location = 0) in vec3 a_position;
      layout(location = 1) in vec3 a_normal;
      layout(location = 2) in vec2 a_uv;
      layout(location = 3) in vec3 a_color;

      out vec3 v_normal;
      out vec3 v_color;

      void main() {
        v_normal = a_normal;
        v_color = a_color;
        gl_Position = u_transform.projection * u_transform.local_to_world * vec4(a_position, 1.0);
      }
    )", GL_VERTEX_SHADER);

    GLuint frag_shader = CreateShader(R"(
      #version 450 core

      layout(location = 0) out vec4 f_frag_color;

      in vec3 v_normal;
      in vec3 v_color;

      void main() {
        f_frag_color = vec4(v_normal * 0.5 + 0.5, 1.0);
      }
    )", GL_FRAGMENT_SHADER);

    m_gl_draw_program = CreateProgram({{vert_shader, frag_shader}});
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
  }

  void OpenGLRenderBackend::CreateDrawListBuilderShaderProgram() {
    GLuint compute_shader = CreateShader(R"(
      #version 450 core

      // TODO(fleroviux): figure out if it makes sense to increase the local workgroup size.
      layout(local_size_x = 1) in;

      struct DrawElementsIndirectCommand {
        uint count;
        uint instance_count;
        uint first_index;
        int  base_vertex;
        uint base_instance;
      };

      layout(std430, binding = 0) readonly buffer GeometryBuffer {
        DrawElementsIndirectCommand rb_geometry_commands[];
      };

      layout(std430, binding = 1) buffer CommandBuffer {
        DrawElementsIndirectCommand b_command_buffer[];
      };

      void main() {
        //b_command_buffer[0] = DrawElementsIndirectCommand(3u, 1u, 0u, 0, 0u);
        b_command_buffer[0] = rb_geometry_commands[0];
      }
    )", GL_COMPUTE_SHADER);

    m_gl_draw_list_builder_program = CreateProgram({{compute_shader}});
    glDeleteShader(compute_shader);
  }

  GLuint OpenGLRenderBackend::CreateShader(const char* glsl_code, GLenum type) {
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

  GLuint OpenGLRenderBackend::CreateProgram(std::span<const GLuint> shaders) {
    GLuint gl_program = glCreateProgram();

    for(auto shader_object : shaders) {
      glAttachShader(gl_program, shader_object);
    }
    glLinkProgram(gl_program);

    GLint link_succeeded;
    glGetProgramiv(gl_program, GL_LINK_STATUS, &link_succeeded);
    if(link_succeeded == GL_FALSE) {
      // TODO(fleroviux): let the caller decide how to deal with the error.
      GLint info_log_length;
      glGetProgramiv(gl_program, GL_INFO_LOG_LENGTH, &info_log_length);

      GLchar* info_log = new GLchar[info_log_length];
      glGetProgramInfoLog(gl_program, info_log_length, &info_log_length, info_log);
      ZEPHYR_PANIC("OpenGL: failed to link GLSL program:\n{}", info_log);
      delete[] info_log;
    }

    return gl_program;
  }

} // namespace zephyr
