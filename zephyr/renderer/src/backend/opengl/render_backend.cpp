
#include "shader/draw_call.glsl.hpp"
#include "shader/draw_list_builder.glsl.hpp"
#include "render_backend.hpp"

namespace zephyr {

  OpenGLRenderBackend::OpenGLRenderBackend(SDL_Window* sdl2_window) : m_window{sdl2_window} {
  }

  void OpenGLRenderBackend::InitializeContext() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    m_gl_context = SDL_GL_CreateContext(m_window);
    if(m_gl_context == nullptr) {
      ZEPHYR_PANIC("Failed to create OpenGL context: {}", SDL_GetError());
    }
    glewInit();

    CreateDrawShaderProgram();
    CreateDrawListBuilderShaderProgram();

    glCreateBuffers(1u, &m_gl_render_bundle_ssbo);
    glNamedBufferStorage(m_gl_render_bundle_ssbo, sizeof(RenderBundleItem) * k_max_draws_per_draw_call, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1u, &m_gl_draw_list_command_ssbo);
    glNamedBufferStorage(m_gl_draw_list_command_ssbo, 6u * sizeof(u32) * k_max_draws_per_draw_call, nullptr, 0);

    glCreateBuffers(1u, &m_gl_camera_ubo);
    glNamedBufferStorage(m_gl_camera_ubo, sizeof(RenderCamera), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1u, &m_gl_draw_count_ubo);
    glNamedBufferStorage(m_gl_draw_count_ubo, sizeof(u32), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1u, &m_gl_draw_count_out_ac);
    glNamedBufferStorage(m_gl_draw_count_out_ac, sizeof(GLuint), nullptr, 0);

    f32 material_data[] = {
      1.0, 0.0, 0.0, 1.0,
      0.0, 1.0, 0.0, 1.0,
      0.0, 0.0, 1.0, 1.0
    };
    glCreateBuffers(1u, &m_gl_material_data_buffer);
    glNamedBufferStorage(m_gl_material_data_buffer, sizeof(material_data), material_data, 0);

    glEnable(GL_DEPTH_TEST);

    m_render_geometry_manager = std::make_unique<OpenGLRenderGeometryManager>();
  }

  void OpenGLRenderBackend::DestroyContext() {
    m_render_geometry_manager.reset();

    glDeleteBuffers(1u, &m_gl_material_data_buffer);
    glDeleteBuffers(1u, &m_gl_draw_count_out_ac);
    glDeleteBuffers(1u, &m_gl_draw_count_ubo);
    glDeleteBuffers(1u, &m_gl_camera_ubo);
    glDeleteBuffers(1u, &m_gl_draw_list_command_ssbo);
    glDeleteBuffers(1u, &m_gl_render_bundle_ssbo);
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

  void OpenGLRenderBackend::UpdateRenderGeometryAABB(RenderGeometry* render_geometry, const Box3& aabb) {
    m_render_geometry_manager->UpdateRenderGeometryAABB(render_geometry, aabb);
  }

  void OpenGLRenderBackend::DestroyRenderGeometry(RenderGeometry* render_geometry) {
    m_render_geometry_manager->DestroyRenderGeometry(render_geometry);
  }

  void OpenGLRenderBackend::Render(const RenderCamera& render_camera, std::span<const RenderObject> render_objects) {
    std::unordered_map<RenderBundleKey, std::vector<RenderBundleItem>> render_bundles;

    for(const RenderObject& render_object : render_objects) {
      // TODO(fleroviux): get rid of unsafe size_t to u32 conversion.
      auto render_geometry = dynamic_cast<OpenGLRenderGeometry*>(render_object.render_geometry);
      RenderBundleKey render_bundle_key{};
      render_bundle_key.uses_ibo = render_geometry->GetNumberOfIndices();
      render_bundle_key.geometry_layout = render_geometry->GetLayout().key;
      render_bundles[render_bundle_key].emplace_back(render_object.local_to_world, (u32)render_geometry->GetGeometryID(), (u32)0u);
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glNamedBufferSubData(m_gl_camera_ubo, 0, sizeof(RenderCamera), &render_camera);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0u, m_gl_camera_ubo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0u, m_gl_render_bundle_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1u, m_gl_draw_list_command_ssbo);

    for(const auto& [key, render_bundle] : render_bundles) {
      const size_t render_bundle_size = render_bundle.size();

      for(size_t base_draw = 0u; base_draw < render_bundle_size; base_draw += k_max_draws_per_draw_call) {
        const u32 number_of_draws = std::min<size_t>(render_bundle_size - base_draw, k_max_draws_per_draw_call);

        // TODO(fleroviux): use persistently mapped buffers (PMBs) for this and see if they are faster?
        // 1. Upload render bundle items into the render bundle buffer
        glNamedBufferSubData(m_gl_render_bundle_ssbo, 0u, (GLsizeiptr)(number_of_draws * sizeof(RenderBundleItem)), &render_bundle[base_draw]);
        glNamedBufferSubData(m_gl_draw_count_ubo, 0u, sizeof(u32), &number_of_draws);

        // 2. Generate multi-draw indirect command buffer from the render bundle buffer and geometry descriptor buffer
        {
          glUseProgram(m_gl_draw_list_builder_program);

          glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2u, m_render_geometry_manager->GetGeometryRenderDataBuffer());
          glBindBufferBase(GL_UNIFORM_BUFFER, 1u, m_gl_draw_count_ubo);
          glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0u, m_gl_draw_count_out_ac);

          const GLuint workgroup_size = 32u;
          const GLuint workgroup_group_count = (number_of_draws + workgroup_size - 1u) / workgroup_size;
          glDispatchCompute(workgroup_group_count, 1u, 1u);

          glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2u, 0u);
          glBindBufferBase(GL_UNIFORM_BUFFER, 1u, 0u);
          glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0u, 0u);
        }

        // 3. Draw everything written to the Draw List SSBO
        {
          glUseProgram(m_gl_draw_program);

          glBindVertexArray(m_render_geometry_manager->GetVAOFromLayout(RenderGeometryLayout{key.geometry_layout}));
          glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_gl_draw_list_command_ssbo);
          glBindBuffer(GL_PARAMETER_BUFFER, m_gl_draw_count_out_ac);
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2u, m_gl_material_data_buffer);

          glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
          if(key.uses_ibo) {
            glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 0u, (GLsizei)number_of_draws, 6u * sizeof(u32));
          } else {
            glMultiDrawArraysIndirectCount(GL_TRIANGLES, nullptr, 0u, (GLsizei)number_of_draws, 6u * sizeof(u32));
          }

          glBindVertexArray(0u);
          glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0u);
          glBindBuffer(GL_PARAMETER_BUFFER, 0u);
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2u, 0u);
        }
      }
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, 0u, 0u);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0u, 0u);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1u, 0u);
  }

  void OpenGLRenderBackend::SwapBuffers() {
    SDL_GL_SwapWindow(m_window);
  }

  void OpenGLRenderBackend::CreateDrawShaderProgram() {
    GLuint vert_shader = CreateShader(k_draw_call_vert_glsl, GL_VERTEX_SHADER);
    GLuint frag_shader = CreateShader(k_draw_call_frag_glsl, GL_FRAGMENT_SHADER);

    m_gl_draw_program = CreateProgram({{vert_shader, frag_shader}});
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
  }

  void OpenGLRenderBackend::CreateDrawListBuilderShaderProgram() {
    GLuint compute_shader = CreateShader(k_draw_list_builder_comp_glsl, GL_COMPUTE_SHADER);

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
