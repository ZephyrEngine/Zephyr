
#include <zephyr/renderer/engine/geometry_cache.hpp>
#include <algorithm>
#include <cstring>

namespace zephyr {

  GeometryCache::~GeometryCache() {
    // Ensure that we do not receive any destruction callbacks from geometries that outlive this geometry cache.
    for(const auto& [geometry, state] : m_geometry_state_table) {
      if(state.uploaded) {
        geometry->OnBeforeDestruct().Unsubscribe(state.destruct_event_subscription);
      }
    }
  }

  void GeometryCache::QueueTasksForRenderThread() {
    // Queue (re-)uploads for all geometries used in the submitted frame which are either new or have changed since the last frame.
    QueueUploadTasksForUsedGeometries();

    // Queue eviction of geometries which had been deleted in the previously submitted frame.
    QueueDeleteTaskFromPreviousFrame();
  }

  void GeometryCache::IncrementGeometryRefCount(const Geometry* geometry) {
    if(++m_geometry_state_table[geometry].ref_count == 1u) {
      m_used_geometry_set.insert(geometry);
    }
  }

  void GeometryCache::DecrementGeometryRefCount(const Geometry* geometry) {
    if(--m_geometry_state_table[geometry].ref_count == 0u) {
      m_used_geometry_set.erase(geometry);
    }
  }

  void GeometryCache::QueueUploadTasksForUsedGeometries() {
    for(const Geometry* geometry : m_used_geometry_set) {
      QueueGeometryUploadTaskIfNeeded(geometry);
    }
  }

  void GeometryCache::QueueDeleteTaskFromPreviousFrame() {
    std::swap(m_delete_tasks[0], m_delete_tasks[1]);
  }

  void GeometryCache::QueueGeometryUploadTaskIfNeeded(const Geometry* geometry) {
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
        state.destruct_event_subscription = geometry->OnBeforeDestruct().Subscribe(
          std::bind(&GeometryCache::QueueGeometryDeleteTaskForNextFrame, this, geometry));
      }

      state.uploaded = true;
      state.current_version = geometry->CurrentVersion();
    }
  }

  void GeometryCache::QueueGeometryDeleteTaskForNextFrame(const Geometry* geometry) {
    /**
     * Queue the geometry for eviction from the cache, when the next frame begin rendering.
     * This ensures that the geometry is only evicted after the current frame has fully rendered.
     */
    m_delete_tasks[1].push_back({.geometry = (const Geometry*)geometry});
    m_geometry_state_table.erase((const Geometry*)geometry);
  }

  void GeometryCache::ProcessQueuedTasks() {
    ProcessQueuedDeleteTasks();
    ProcessQueuedUploadTasks();
  }

  void GeometryCache::ProcessQueuedDeleteTasks() {
    for(const auto& delete_task : m_delete_tasks[0]) {
      RenderGeometry* render_geometry = m_render_geometry_table[delete_task.geometry];
      if(render_geometry) {
        m_render_backend->DestroyRenderGeometry(render_geometry);
      }
      m_render_geometry_table.erase(delete_task.geometry);
    }

    m_delete_tasks[0].clear();
  }

  void GeometryCache::ProcessQueuedUploadTasks() {
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
