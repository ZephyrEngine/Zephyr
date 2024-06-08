
#pragma once

#include <zephyr/math/frustum.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/engine/geometry_cache.hpp>
#include <zephyr/renderer/resource/geometry.hpp>
#include <zephyr/scene/scene_graph.hpp>
#include <zephyr/integer.hpp>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <memory>
#include <vector>

namespace zephyr {

  class RenderScene {
    public:
      void SetSceneGraph(std::shared_ptr<SceneGraph> scene_graph);
      void Update();
      void GetRenderCamera(RenderCamera& out_render_camera);
      void UpdateGeometries(GeometryCache& geometry_cache);

      void UpdateRenderBundles(const GeometryCache& geometry_cache);

      [[nodiscard]] const eastl::hash_map<RenderBackend::RenderBundleKey, std::vector<RenderBackend::RenderBundleItem>>& GetRenderBundles() {
        return m_render_bundles;
      }

    private:
      using Entity = u32;
      using EntityID = size_t;

      enum ComponentFlag : Entity {
        COMPONENT_FLAG_MESH = 1ul << 0,
        COMPONENT_FLAG_CAMERA = 1ul << 1
      };

      struct Transform {
        Matrix4 local_to_world;
      };

      struct Mesh {
        const Geometry* geometry;
      };

      struct Camera {
        Matrix4 projection;
        Frustum frustum;
      };

      struct RenderScenePatch {
        enum class Type : u8 {
          MeshMounted,
          MeshRemoved,
          TransformChanged
        };

        Type type;
        EntityID entity_id;
      };

      struct RenderBundleItemLocation {
        RenderBackend::RenderBundleKey key;
        size_t index;
      };

      void RebuildScene();
      void PatchScene();
      void PatchNodeMounted(SceneNode* node);
      void PatchNodeRemoved(SceneNode* node);
      void PatchNodeComponentMounted(SceneNode* node, std::type_index component_type);
      void PatchNodeComponentRemoved(SceneNode* node, std::type_index component_type);
      void PatchNodeTransformChanged(SceneNode* node);

      EntityID GetOrCreateEntityForNode(const SceneNode* node);

      EntityID CreateEntity();
      void DestroyEntity(EntityID entity_id);
      void ResizeComponentStorage(size_t capacity);

      std::shared_ptr<SceneGraph> m_current_scene_graph{};
      eastl::hash_map<const SceneNode*, EntityID> m_node_entity_map{};
      eastl::hash_set<const Geometry*> m_active_geometry_set{};
      bool m_require_full_rebuild{};

      std::vector<Entity> m_entities{};
      std::vector<EntityID> m_free_entity_list{};
      std::vector<Transform> m_components_transform{};
      std::vector<Mesh> m_components_mesh{};
      std::vector<Camera> m_components_camera{};
      std::vector<EntityID> m_view_mesh{};
      std::vector<EntityID> m_view_camera{};

      std::vector<RenderScenePatch> m_render_scene_patches{};
      eastl::hash_map<EntityID, RenderBundleItemLocation> m_entity_to_render_item_location{};
      eastl::hash_map<RenderBackend::RenderBundleKey, std::vector<RenderBackend::RenderBundleItem>> m_render_bundles{};
  };

} // namespace zephyr