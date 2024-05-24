
#include "render_geometry.hpp"

namespace zephyr {

  OpenGLRenderGeometry::OpenGLRenderGeometry(
    RenderGeometryLayout layout,
    size_t number_of_vertices,
    size_t number_of_indices,
    std::shared_ptr<OpenGLDynamicGPUArray> vbo,
    std::shared_ptr<OpenGLDynamicGPUArray> ibo,
    std::shared_ptr<OpenGLDynamicGPUArray> draw_command_buffer
  )   : m_layout{layout}
      , m_vbo{std::move(vbo)}
      , m_geometry_render_data_buffer{std::move(draw_command_buffer)} {
    m_vbo_allocation = m_vbo->AllocateRange(number_of_vertices);
    m_geometry_render_data_allocation = m_geometry_render_data_buffer->AllocateRange(1u);

    if(number_of_indices > 0u) {
      m_ibo = std::move(ibo);
      m_ibo_allocation = m_ibo->AllocateRange(number_of_indices);

      // See DrawElementsIndirectCommand structure definition:
      // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMultiDrawElementsIndirect.xhtml
      m_geometry_render_data.mdi_command[0] = (u32)number_of_indices;
      m_geometry_render_data.mdi_command[1] = 1u; // Number of instances
      m_geometry_render_data.mdi_command[2] = (u32)m_ibo_allocation.base_element;
      m_geometry_render_data.mdi_command[3] = (u32)m_vbo_allocation.base_element;
      m_geometry_render_data.mdi_command[4] = 0u; // Base instance
    } else {
      // See DrawArraysIndirectCommand structure definition:
      // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMultiDrawArraysIndirect.xhtml
      m_geometry_render_data.mdi_command[0] = (u32)number_of_vertices;
      m_geometry_render_data.mdi_command[1] = 1u; // Number of instances
      m_geometry_render_data.mdi_command[2] = (u32)m_vbo_allocation.base_element;
      m_geometry_render_data.mdi_command[3] = 0u; // Base instance
    }

    m_geometry_render_data.aabb_min = {-std::numeric_limits<f32>::infinity(), -std::numeric_limits<f32>::infinity(), -std::numeric_limits<f32>::infinity(), 0};
    m_geometry_render_data.aabb_max = { std::numeric_limits<f32>::infinity(),  std::numeric_limits<f32>::infinity(),  std::numeric_limits<f32>::infinity(), 0};
    WriteGeometryRenderDataToBuffer();
  }

  OpenGLRenderGeometry::~OpenGLRenderGeometry() {
    m_vbo->ReleaseRange(m_vbo_allocation);
    if(m_ibo) {
      m_ibo->ReleaseRange(m_ibo_allocation);
    }
    m_geometry_render_data_buffer->ReleaseRange(m_geometry_render_data_allocation);
  }

  void OpenGLRenderGeometry::WriteVBO(std::span<const u8> data) {
    // TODO(fleroviux): this does not protect against writing into neighbouring allocations.
    m_vbo->Write(data, m_vbo_allocation.base_element);
  }

  void OpenGLRenderGeometry::WriteIBO(std::span<const u8> data) {
    // TODO(fleroviux): this does not protect against writing into neighbouring allocations.
    if(!m_ibo) {
      ZEPHYR_PANIC("Attempted to write IBO of non-indexed render geometry");
    }
    m_ibo->Write(data, m_ibo_allocation.base_element);
  }

  void OpenGLRenderGeometry::SetAABB(const Box3& aabb) {
    m_geometry_render_data.aabb_min.X() = aabb.Min().X();
    m_geometry_render_data.aabb_min.Y() = aabb.Min().Y();
    m_geometry_render_data.aabb_min.Z() = aabb.Min().Z();
    m_geometry_render_data.aabb_max.X() = aabb.Max().X();
    m_geometry_render_data.aabb_max.Y() = aabb.Max().Y();
    m_geometry_render_data.aabb_max.Z() = aabb.Max().Z();
    WriteGeometryRenderDataToBuffer();
  }

  void OpenGLRenderGeometry::WriteGeometryRenderDataToBuffer() {
    m_geometry_render_data_buffer->Write({(const u8*)&m_geometry_render_data, sizeof(m_geometry_render_data)}, m_geometry_render_data_allocation.base_element);
  }

} // namespace zephyr
