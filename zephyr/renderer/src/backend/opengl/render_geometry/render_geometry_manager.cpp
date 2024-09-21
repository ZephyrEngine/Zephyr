
#include "render_geometry_manager.hpp"

namespace zephyr {

OpenGLRenderGeometryManager::OpenGLRenderGeometryManager() {
  m_ibo = std::make_shared<OpenGLDynamicGPUArray>(sizeof(u32));
  m_geometry_render_data = std::make_shared<OpenGLDynamicGPUArray>(sizeof(OpenGLRenderGeometry::RenderData));
}

GLuint OpenGLRenderGeometryManager::GetVAOFromLayout(RenderGeometryLayout layout) {
  // TODO(fleroviux): do not create the bucket if it does not exist.
  // TODO(fleroviux): avoid unnecessary rebinding of the vertex and index buffers.
  Bucket& bucket = GetBucketFromLayout(layout);
  glVertexArrayVertexBuffer(bucket.vao, 0u, bucket.vbo->GetBufferHandle(), 0u, (GLsizei)bucket.vbo->GetByteStride());
  glVertexArrayElementBuffer(bucket.vao, m_ibo->GetBufferHandle());
  return bucket.vao;
}

GLuint OpenGLRenderGeometryManager::GetGeometryRenderDataBuffer() {
  return m_geometry_render_data->GetBufferHandle();
}

RenderGeometry* OpenGLRenderGeometryManager::CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) {
  const Bucket& bucket = GetBucketFromLayout(layout);
  return new OpenGLRenderGeometry{layout, number_of_vertices, number_of_indices, bucket.vbo, m_ibo, m_geometry_render_data};
}

void OpenGLRenderGeometryManager::UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) {
  dynamic_cast<OpenGLRenderGeometry*>(render_geometry)->WriteIBO(data);
}

void OpenGLRenderGeometryManager::UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) {
  dynamic_cast<OpenGLRenderGeometry*>(render_geometry)->WriteVBO(data);
}

void OpenGLRenderGeometryManager::UpdateRenderGeometryAABB(RenderGeometry* render_geometry, const Box3& aabb) {
  dynamic_cast<OpenGLRenderGeometry*>(render_geometry)->SetAABB(aabb);
}

void OpenGLRenderGeometryManager::DestroyRenderGeometry(RenderGeometry* render_geometry) {
  delete render_geometry;
}

OpenGLRenderGeometryManager::Bucket& OpenGLRenderGeometryManager::GetBucketFromLayout(RenderGeometryLayout layout) {
  Bucket& bucket = m_layout_to_bucket_table[layout.key];

  if(!bucket.vbo) {
    size_t next_attribute_offset = 0u;

    const auto RegisterAttribute = [&](RenderGeometryAttribute attribute, int number_of_components) {
      if(!layout.HasAttribute(attribute)) {
        return;
      }
      glEnableVertexArrayAttrib(bucket.vao, (int)attribute);
      glVertexArrayAttribFormat(bucket.vao, (int)attribute, number_of_components, GL_FLOAT, GL_FALSE, next_attribute_offset);
      glVertexArrayAttribBinding(bucket.vao, (int)attribute, 0u);
      next_attribute_offset += sizeof(f32) * number_of_components;
    };

    glCreateVertexArrays(1u, &bucket.vao);
    RegisterAttribute(RenderGeometryAttribute::Position, 3);
    RegisterAttribute(RenderGeometryAttribute::Normal, 3);
    RegisterAttribute(RenderGeometryAttribute::UV, 2);
    RegisterAttribute(RenderGeometryAttribute::Color, 4);

    const size_t byte_stride = next_attribute_offset;
    bucket.vbo = GetVBOFromByteStride(byte_stride);
  }

  return bucket;
}

std::shared_ptr<OpenGLDynamicGPUArray> OpenGLRenderGeometryManager::GetVBOFromByteStride(size_t byte_stride) {
  if(!m_layout_to_bucket_table.contains(byte_stride)) {
    m_byte_stride_to_vbo_table[byte_stride] = std::make_shared<OpenGLDynamicGPUArray>(byte_stride);
  }
  return m_byte_stride_to_vbo_table[byte_stride];
}

} // namespace zephyr
