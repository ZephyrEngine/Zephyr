
#pragma once

#include <zephyr/renderer/backend/render_backend_mgpu.hpp>
#include <mgpu/mgpu.h>
#include <vector>

namespace zephyr {

class MGPURenderBackend final : public RenderBackend {
  public:
    explicit MGPURenderBackend(MGPUInstance mgpu_instance, MGPUSurface mgpu_surface);

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
    void CreateSwapChain();
    void DestroySwapChain();

    MGPUInstance m_mgpu_instance;
    MGPUSurface m_mgpu_surface;

    MGPUPhysicalDevice m_mgpu_physical_device{};
    MGPUDevice m_mgpu_device{};
    MGPUQueue m_mgpu_queue{};

    MGPUCommandList m_mgpu_command_list{};

    MGPUSwapChain m_mgpu_swap_chain{};
    std::vector<MGPUTextureView> m_mgpu_swap_chain_texture_views{};
    MGPUTexture m_mgpu_depth_texture{};
    MGPUTextureView m_mgpu_depth_texture_view{};
};

std::unique_ptr<RenderBackend> CreateMGPURenderBackend(MGPUInstance mgpu_instance, MGPUSurface mgpu_surface) {
  return std::make_unique<MGPURenderBackend>(mgpu_instance, mgpu_surface);
}

} // namespace zephyr
