
#include <zephyr/renderer/engine/geometry_cache.hpp>
#include <zephyr/panic.hpp>
#include <algorithm>
#include <cstring>

namespace zephyr {

  GeometryCache::~GeometryCache() {
    // Ensure that we do not receive any destruction callbacks from geometries that outlive this geometry cache.
    for(const auto& [geometry, state] : m_geometry_state_table) {
      geometry->OnBeforeDestruct().Unsubscribe(state.destruct_event_subscription);
    }
  }

  void GeometryCache::CommitPendingDeleteTaskList() {
    std::swap(m_delete_tasks[0], m_delete_tasks[1]);
  }

  void GeometryCache::UpdateGeometry(const Geometry* geometry) {
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
        .aabb = geometry->GetAABB(),
        .raw_vbo_data = CopyDataToStagingBuffer(geometry->GetRawVertexData()),
        .raw_ibo_data = CopyDataToStagingBuffer(geometry->GetRawIndexData()),
        .layout = geometry->GetLayout(),
        .number_of_vertices = geometry->GetNumberOfVertices(),
        .number_of_indices  = geometry->GetNumberOfIndices()
      });

      if(!state.uploaded) {
        state.destruct_event_subscription = geometry->OnBeforeDestruct().Subscribe([this, geometry]() {
          /**
           * This callback is called from the game thread and may be called outside the frame submission phase.
           * To avoid deleting geometries too early, we have to push the delete task to an intermediate list,
           * which is then committed for execution for the next frame submission.
           */
          m_delete_tasks[1].push_back({.geometry = (const Geometry*)geometry});
          m_geometry_state_table.erase((const Geometry*)geometry);
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
    if(!m_render_geometry_table.contains(geometry)) {
      ZEPHYR_PANIC("Bad attempt to retrieve cached render geometry of a geometry which isn't cached.")
    }
    return m_render_geometry_table[geometry];
  }

  void GeometryCache::ProcessPendingDeletes() {
    for(const auto& delete_task : m_delete_tasks[0]) {
      RenderGeometry* render_geometry = m_render_geometry_table[delete_task.geometry];
      if(render_geometry) {
        m_render_backend->DestroyRenderGeometry(render_geometry);
      }
      m_render_geometry_table.erase(delete_task.geometry);
    }

    m_delete_tasks[0].clear();
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
      if(new_number_of_indices > 0) {
        m_render_backend->UpdateRenderGeometryIndices(render_geometry, upload_task.raw_ibo_data);
      }
      m_render_backend->UpdateRenderGeometryAABB(render_geometry, upload_task.aabb);

      delete[] upload_task.raw_vbo_data.data();
      delete[] upload_task.raw_ibo_data.data();
    }

    m_upload_tasks.clear();
  }

} // namespace zephyr
