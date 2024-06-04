
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/engine/geometry_cache.hpp>
#include <zephyr/renderer/resource/geometry.hpp>
#include <zephyr/scene/scene_graph.hpp>
#include <zephyr/scene/scene_node.hpp>
#include <EASTL/hash_map.h>
#include <atomic>
#include <optional>
#include <semaphore>
#include <thread>
#include <typeindex>
#include <vector>

namespace zephyr {

  class RenderEngine {
    public:
      explicit RenderEngine(std::unique_ptr<RenderBackend> render_backend);
     ~RenderEngine();

      void SetSceneGraph(std::shared_ptr<SceneGraph> scene_graph);
      void RenderScene();

    private:
      struct SceneNodeMeshData {
        SceneNodeMeshData(const Matrix4& local_to_world, Geometry* geometry) : local_to_world{local_to_world}, geometry{geometry} {}
        Matrix4 local_to_world;
        Geometry* geometry;
      };

      struct SceneNodeData {
        std::optional<size_t> mesh_data_id{};
        std::optional<size_t> camera_data_id{};

        [[nodiscard]] bool Empty() const {
          return !mesh_data_id.has_value() &&
                 !camera_data_id.has_value();
        }
      };

      // Methods used for translating the scene graph into our internal representation.
      void RebuildScene();
      void PatchScene();
      void PatchNodeMounted(SceneNode* node);
      void PatchNodeRemoved(SceneNode* node);
      void PatchNodeComponentMounted(SceneNode* node, std::type_index component_type);
      void PatchNodeComponentRemoved(SceneNode* node, std::type_index component_type);
      void PatchNodeTransformChanged(SceneNode* node);

      void CreateRenderThread();
      void JoinRenderThread();
      void RenderThreadMain();
      void ReadyRenderThreadData();

      std::shared_ptr<RenderBackend> m_render_backend;

      std::thread m_render_thread;
      std::atomic_bool m_render_thread_running;
      std::atomic_bool m_render_thread_is_waiting;
      std::binary_semaphore m_caller_thread_semaphore{0}; //> Semaphore signalled by the calling thread
      std::binary_semaphore m_render_thread_semaphore{1}; //> Semaphore signalled by the rendering thread

      GeometryCache m_geometry_cache;

      std::shared_ptr<SceneGraph> m_current_scene_graph{};
      bool m_need_scene_rebuild{};

      // Representation of the scene graph that is internal to the render engine.
      std::vector<SceneNodeMeshData> m_scene_node_mesh_data{};
      std::vector<RenderCamera> m_scene_node_camera_data{};
      eastl::hash_map<const SceneNode*, SceneNodeData> m_scene_node_data{};

      std::vector<RenderObject> m_render_objects{};
      RenderCamera m_render_camera{};
  };

} // namespace zephyr
