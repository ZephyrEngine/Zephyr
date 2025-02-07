
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/panic.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "render_geometry/render_geometry_manager.hpp"
#include "render_texture/render_texture_manager.hpp"

namespace zephyr {

class OpenGLRenderBackend final : public RenderBackend {
  public:
    explicit OpenGLRenderBackend(SDL_Window* sdl2_window);

    void InitializeContext() override;
    void DestroyContext() override;

    RenderGeometry* CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) override;
    void UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) override;
    void UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) override;
    void UpdateRenderGeometryAABB(RenderGeometry* render_geometry, const Box3& aabb) override;
    void DestroyRenderGeometry(RenderGeometry* render_geometry) override;

    RenderTexture* CreateRenderTexture(u32 width, u32 height) override;
    void UpdateRenderTextureData(RenderTexture* render_texture, std::span<const u8> data) override;
    void DestroyRenderTexture(RenderTexture* render_texture) override;

    void Render(const RenderCamera& render_camera, const eastl::hash_map<RenderBundleKey, std::vector<RenderBundleItem>>& render_bundles) override;

    void SwapBuffers() override;

  private:
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
    std::unique_ptr<OpenGLRenderTextureManager> m_render_texture_manager{};
};

std::unique_ptr<RenderBackend> CreateOpenGLRenderBackendForSDL2(SDL_Window* sdl2_window) {
  return std::make_unique<OpenGLRenderBackend>(sdl2_window);
}

} // namespace zephyr
