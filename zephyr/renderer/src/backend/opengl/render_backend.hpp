
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/hash.hpp>
#include <zephyr/panic.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <unordered_map>

#include "render_geometry.hpp"

namespace zephyr {

  class OpenGLRenderBackend final : public RenderBackend {
    public:
      struct RenderBundleKey {
        bool uses_ibo;
        u32 geometry_layout;

        [[nodiscard]] bool operator==(const RenderBundleKey& other) const {
          return uses_ibo == other.uses_ibo && geometry_layout == other.geometry_layout;
        }
      };

      explicit OpenGLRenderBackend(SDL_Window* sdl2_window);

      void InitializeContext() override;
      void DestroyContext() override;

      RenderGeometry* CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) override;
      void UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) override;
      void UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) override;
      void UpdateRenderGeometryAABB(RenderGeometry* render_geometry, const Box3& aabb) override;
      void DestroyRenderGeometry(RenderGeometry* render_geometry) override;

      void Render(const RenderCamera& render_camera, std::span<const RenderObject> render_objects) override;

      void SwapBuffers() override;

    private:
      struct RenderBundleItem {
        RenderBundleItem(const Matrix4& local_to_world, u32 draw_command_id, u32 material_id) : local_to_world{local_to_world}, draw_command_id{draw_command_id}, material_id{material_id} {}
        Matrix4 local_to_world;
        u32 draw_command_id;
        u32 material_id;
        u32 padding[2]; // Padding for std430 buffer layout
      };

      static constexpr u32 k_max_draws_per_draw_call = 16384;

      void CreateDrawShaderProgram();
      void CreateDrawListBuilderShaderProgram();

      static GLuint CreateShader(const char* glsl_code, GLenum type);
      static GLuint CreateProgram(std::span<const GLuint> shaders);

      SDL_Window* m_window;
      SDL_GLContext m_gl_context{};
      GLuint m_gl_draw_program{};
      GLuint m_gl_draw_list_builder_program{};
      GLuint m_gl_render_bundle_ssbo{};
      GLuint m_gl_draw_list_command_ssbo{};
      GLuint m_gl_camera_ubo{};
      GLuint m_gl_draw_count_ubo{};
      GLuint m_gl_draw_count_out_ac{};

      GLuint m_gl_material_data_buffer{};

      std::unique_ptr<OpenGLRenderGeometryManager> m_render_geometry_manager{};
  };

  std::unique_ptr<RenderBackend> CreateOpenGLRenderBackendForSDL2(SDL_Window* sdl2_window) {
    return std::make_unique<OpenGLRenderBackend>(sdl2_window);
  }

} // namespace zephyr

template<>
struct std::hash<zephyr::OpenGLRenderBackend::RenderBundleKey> {
  [[nodiscard]] std::size_t operator()(const zephyr::OpenGLRenderBackend::RenderBundleKey& key) const noexcept {
    size_t h = 0;
    zephyr::hash_combine(h, key.uses_ibo);
    zephyr::hash_combine(h, key.geometry_layout);
    return h;
  }
};