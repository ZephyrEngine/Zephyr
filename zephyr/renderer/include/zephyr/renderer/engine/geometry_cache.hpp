
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/resource/geometry.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace zephyr {

  class GeometryCache {
    public:
      explicit GeometryCache(std::shared_ptr<RenderBackend> render_backend) : m_render_backend{std::move(render_backend)} {}

      void ScheduleUploadIfNeeded(const Geometry* geometry);
      void ProcessPendingUpdates();
      RenderGeometry* GetCachedRenderGeometry(const Geometry* geometry);

    private:
      struct GeometryState {
        bool uploaded{false};
        u64 current_version{};
      };

      struct UploadTask {
        const Geometry* geometry;
        std::span<const u8> raw_vbo_data;
        std::span<const u8> raw_ibo_data;
        RenderGeometryLayout layout;
        size_t number_of_vertices;
        size_t number_of_indices;
      };

      struct DeleteTask {
        const Geometry* geometry;
      };

      void ProcessPendingDeletes();
      void ProcessPendingUploads();

      std::shared_ptr<RenderBackend> m_render_backend;
      std::unordered_map<const Geometry*, GeometryState> m_geometry_state_table{};
      std::unordered_map<const Geometry*, RenderGeometry*> m_render_geometry_table{};
      std::vector<UploadTask> m_upload_tasks{};
      std::vector<DeleteTask> m_delete_tasks{};
  };

} // namespace zephyr
