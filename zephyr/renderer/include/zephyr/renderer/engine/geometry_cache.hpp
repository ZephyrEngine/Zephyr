
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/resource/geometry.hpp>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <memory>
#include <vector>

namespace zephyr {

  class GeometryCache {
    public:
      explicit GeometryCache(std::shared_ptr<RenderBackend> render_backend) : m_render_backend{std::move(render_backend)} {}

     ~GeometryCache();

      // Game Thread API:
      void QueueTasksForRenderThread();
      void IncrementGeometryRefCount(const Geometry* geometry);
      void DecrementGeometryRefCount(const Geometry* geometry);

      // Render Thread API:
      void ProcessQueuedTasks();

      RenderGeometry* GetCachedRenderGeometry(const Geometry* geometry) const {
        const auto match = m_render_geometry_table.find(geometry);
        if(match == m_render_geometry_table.end()) {
          ZEPHYR_PANIC("Bad attempt to retrieve cached render geometry of a geometry which isn't cached.")
        }
        return match->second;
      }

    private:
      struct GeometryState {
        bool uploaded{false};
        u64 current_version{};
        size_t ref_count{};
        VoidEvent::SubID destruct_event_subscription;
      };

      struct UploadTask {
        const Geometry* geometry;
        Box3 aabb;
        std::span<const u8> raw_vbo_data;
        std::span<const u8> raw_ibo_data;
        RenderGeometryLayout layout;
        size_t number_of_vertices;
        size_t number_of_indices;
      };

      struct DeleteTask {
        const Geometry* geometry;
      };

      // Game Thread:
      void QueueUploadTasksForUsedGeometries();
      void QueueDeleteTaskFromPreviousFrame();
      void QueueGeometryUploadTaskIfNeeded(const Geometry* geometry);
      void QueueGeometryDeleteTaskForNextFrame(const Geometry* geometry);

      // Render Thread:
      void ProcessQueuedDeleteTasks();
      void ProcessQueuedUploadTasks();

      std::shared_ptr<RenderBackend> m_render_backend;
      eastl::hash_set<const Geometry*> m_used_geometry_set{};
      eastl::hash_map<const Geometry*, GeometryState> m_geometry_state_table{};
      mutable eastl::hash_map<const Geometry*, RenderGeometry*> m_render_geometry_table{};
      std::vector<UploadTask> m_upload_tasks{};
      std::vector<DeleteTask> m_delete_tasks[2]{};
  };

} // namespace zephyr
