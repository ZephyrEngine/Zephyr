
#include <zephyr/renderer/engine/geometry_cache.hpp>
#include <cstring>

namespace zephyr {

  void GeometryCache::ScheduleUploadIfNeeded(const Geometry* geometry) {
    GeometryState& state = m_geometry_state_table[geometry];

    if(!state.uploaded || state.current_version != geometry->CurrentVersion()) {
      const auto CopyDataToStagingBuffer = [](std::span<const u8> data) -> std::span<const u8> {
        // TODO(fleroviux): allocate staging memory from a dedicated allocator?
        u8* staging_data = new u8[data.size_bytes()];
        std::memcpy(staging_data, data.data(), data.size_bytes());
        return {staging_data, data.size_bytes()};
      };

      m_upload_tasks.push_back({
        .geometry = geometry,
        .raw_vbo_data = CopyDataToStagingBuffer(geometry->GetRawVertexData()),
        .raw_ibo_data = CopyDataToStagingBuffer(geometry->GetRawIndexData()),
        .layout = geometry->GetLayout(),
        .number_of_vertices = geometry->GetNumberOfVertices(),
        .number_of_indices  = geometry->GetNumberOfIndices()
      });

      if(!state.uploaded) {
        geometry->RegisterDeleteCallback([this](const Resource* geometry) {
          m_geometry_state_table.erase((const Geometry*)geometry);
          m_delete_tasks.push_back({.geometry = (const Geometry*)geometry});
        });
      }

      state.uploaded = true;
      state.current_version = geometry->CurrentVersion();
    }
  }

  void GeometryCache::ProcessPendingUpdates() {
    ProcessPendingDeletes();
    ProcessPendingUploads();
  }

  RenderGeometry* GeometryCache::GetCachedRenderGeometry(const Geometry* geometry) {
    return m_render_geometry_table[geometry];
  }

  void GeometryCache::ProcessPendingDeletes() {
    for(const auto& delete_task : m_delete_tasks) {
      RenderGeometry* render_geometry = m_render_geometry_table[delete_task.geometry];
      if(render_geometry) {
        m_render_backend->DestroyRenderGeometry(render_geometry);
      }
      m_render_geometry_table.erase(delete_task.geometry);
    }

    m_delete_tasks.clear();
  }

  void GeometryCache::ProcessPendingUploads() {
    for(const auto& upload_task : m_upload_tasks) {
      const Geometry* geometry = upload_task.geometry;
      RenderGeometry* render_geometry = m_render_geometry_table[geometry];

      const size_t new_number_of_vertices = upload_task.number_of_vertices;
      const size_t new_number_of_indices  = upload_task.number_of_indices;

      if(!render_geometry || new_number_of_vertices != render_geometry->GetNumberOfVertices() || new_number_of_indices != render_geometry->GetNumberOfIndices()) {
        if(render_geometry) {
          m_render_backend->DestroyRenderGeometry(render_geometry);
        }
        render_geometry = m_render_backend->CreateRenderGeometry(upload_task.layout, new_number_of_vertices, new_number_of_indices);
        m_render_geometry_table[geometry] = render_geometry;
      }

      m_render_backend->UpdateRenderGeometryVertices(render_geometry, upload_task.raw_vbo_data);
      m_render_backend->UpdateRenderGeometryIndices(render_geometry, upload_task.raw_ibo_data);

      delete[] upload_task.raw_vbo_data.data();
      delete[] upload_task.raw_ibo_data.data();
    }

    m_upload_tasks.clear();
  }

} // namespace zephyr
