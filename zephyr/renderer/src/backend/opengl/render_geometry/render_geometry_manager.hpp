
#pragma once

#include <zephyr/math/box3.hpp>
#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/integer.hpp>
#include <memory>
#include <span>
#include <unordered_map>

#include "backend/opengl/dynamic_gpu_array.hpp"
#include "render_geometry.hpp"

namespace zephyr {

class OpenGLRenderGeometryManager {
  public:
    OpenGLRenderGeometryManager();

    GLuint GetVAOFromLayout(RenderGeometryLayout layout);
    GLuint GetGeometryRenderDataBuffer();

    RenderGeometry* CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices);

    void UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data);
    void UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data);
    void UpdateRenderGeometryAABB(RenderGeometry* render_geometry, const Box3& aabb);
    void DestroyRenderGeometry(RenderGeometry* render_geometry);

  private:
    struct Bucket {
     ~Bucket() { glDeleteVertexArrays(1u, &vao); }
      GLuint vao{};
      std::shared_ptr<OpenGLDynamicGPUArray> vbo{};
    };

    Bucket& GetBucketFromLayout(RenderGeometryLayout layout);
    std::shared_ptr<OpenGLDynamicGPUArray> GetVBOFromByteStride(size_t byte_stride);

    std::shared_ptr<OpenGLDynamicGPUArray> m_ibo{};
    std::unordered_map<size_t, std::shared_ptr<OpenGLDynamicGPUArray>> m_byte_stride_to_vbo_table{};
    std::unordered_map<decltype(RenderGeometryLayout::key), Bucket> m_layout_to_bucket_table{};
    std::shared_ptr<OpenGLDynamicGPUArray> m_geometry_render_data{};
};

} // namespace zephyr