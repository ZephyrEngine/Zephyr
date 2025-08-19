
#include <zephyr/renderer/component/camera.hpp>
#include <zephyr/renderer/component/mesh.hpp>
#include <zephyr/renderer/render_scene.hpp>
#include <zephyr/scene/scene_node.hpp>
#include <algorithm>

namespace zephyr {

RenderScene::RenderScene(std::shared_ptr<RenderBackend> render_backend)
    : m_geometry_cache{render_backend}
    , m_texture_cache{render_backend}
    , m_material_cache{std::move(render_backend), m_texture_cache} {
}

void RenderScene::SetSceneGraph(std::shared_ptr<SceneGraph> scene_graph) {
  if(m_current_scene_graph != scene_graph) {
    m_current_scene_graph = std::move(scene_graph);
    m_require_full_rebuild = true;
  }
}

void RenderScene::UpdateStage1() {
  if(m_require_full_rebuild) {
    RebuildScene();
    m_require_full_rebuild = false;
  } else {
    PatchScene();
  }

/*
  // Temporary: test creating a texture and uploading some data to it
  if(!m_test_texture) {
    m_test_texture = std::make_unique<Texture2D>(64, 64);

    // Create a basic checkerboard pattern
    u32* dst_pixel = m_test_texture->Data<u32>();
    for(int y = 0; y < 64; y++) {
      for(int x = 0; x < 64; x++) {
        *dst_pixel++ = ((x ^ y) & 8) ? 0xFFFF0000u : 0xFFFFFFFFu;
      }
    }

    m_texture_cache.IncrementTextureRefCount(m_test_texture.get());
  }
*/

  // Queue geometry cache updates and evictions to be processed on the render thread.
  m_geometry_cache.QueueTasksForRenderThread();

  // Queue texture cache updates and evictions to be processed on the render thread.
  m_texture_cache.QueueTasksForRenderThread();
}

void RenderScene::GetRenderCamera(RenderCamera& out_render_camera) {
  // TODO(fleroviux): implement a better way to pick the camera to use.
  if(m_view_camera.empty()) {
    ZEPHYR_PANIC("Scene graph does not contain a camera to render with.");
  }

  const EntityID entity_id = m_view_camera[0];
  const Transform& entity_transform = m_components_transform[entity_id];
  const Camera& entity_camera = m_components_camera[entity_id];
  out_render_camera.projection = entity_camera.projection;
  out_render_camera.frustum = entity_camera.frustum;
  out_render_camera.view = entity_transform.local_to_world.Inverse();
}

[[nodiscard]] const eastl::hash_map<RenderBackend::RenderBundleKey, std::vector<RenderBackend::RenderBundleItem>>& RenderScene::GetRenderBundles() {
  return m_render_bundles;
}

void RenderScene::UpdateStage2() {
  m_geometry_cache.ProcessQueuedTasks();
  m_texture_cache.ProcessQueuedTasks();

  for(const RenderScenePatch& render_scene_patch : m_render_scene_patches) {
    switch(render_scene_patch.type) {
      case RenderScenePatch::Type::MeshMounted: {
        const EntityID entity_id = render_scene_patch.entity_id;
        const Transform& entity_transform = m_components_transform[entity_id];
        const Mesh& entity_mesh = m_components_mesh[render_scene_patch.entity_id];

        // TODO(fleroviux): get rid of unsafe size_t to u32 conversion.
        const RenderGeometry* const render_geometry = m_geometry_cache.GetCachedRenderGeometry(entity_mesh.geometry);
        RenderBackend::RenderBundleKey render_bundle_key{};
        render_bundle_key.uses_ibo = render_geometry->GetNumberOfIndices();
        render_bundle_key.geometry_layout = render_geometry->GetLayout().key;

        std::vector<RenderBackend::RenderBundleItem>& render_bundle = m_render_bundles[render_bundle_key];
        render_bundle.emplace_back(entity_transform.local_to_world, (u32)render_geometry->GetGeometryID(), (u32)0u, entity_id);

        m_entity_to_render_item_location[entity_id] = { render_bundle_key, render_bundle.size() - 1u };
        break;
      }
      case RenderScenePatch::Type::MeshRemoved: {
        const auto match = m_entity_to_render_item_location.find(render_scene_patch.entity_id);
        const RenderBundleItemLocation& location = match->second;

        std::vector<RenderBackend::RenderBundleItem>& render_bundle = m_render_bundles[location.key];
        render_bundle[location.index] = render_bundle.back();
        m_entity_to_render_item_location[render_bundle.back().entity_id].index = location.index;
        render_bundle.pop_back();

        m_entity_to_render_item_location.erase(match);
        break;
      }
      case RenderScenePatch::Type::TransformChanged: {
        const auto match = m_entity_to_render_item_location.find(render_scene_patch.entity_id);

        if(match != m_entity_to_render_item_location.end()) {
          const RenderBundleItemLocation& location = match->second;

          m_render_bundles[location.key][location.index].local_to_world = m_components_transform[render_scene_patch.entity_id].local_to_world;
        }
        break;
      }
      default: ZEPHYR_PANIC("unhandled patch type: {}", (int)render_scene_patch.type);
    }
  }

  m_render_scene_patches.clear();
}

void RenderScene::RebuildScene() {
  m_node_entity_map.clear();
  m_entities.clear();
  ResizeComponentStorage(0);

  m_current_scene_graph->GetRoot()->Traverse([this](SceneNode* child_node) {
    if(child_node->IsVisible()) {
      PatchNodeMounted(child_node);
      return true;
    }
    return false;
  });
}

void RenderScene::PatchScene() {
  for(const ScenePatch& patch : m_current_scene_graph->GetScenePatches()) {
    switch(patch.type) {
      case ScenePatch::Type::NodeMounted: PatchNodeMounted(patch.node.get()); break;
      case ScenePatch::Type::NodeRemoved: PatchNodeRemoved(patch.node.get()); break;
      case ScenePatch::Type::ComponentMounted: PatchNodeComponentMounted(patch.node.get(), patch.component_type); break;
      case ScenePatch::Type::ComponentRemoved: PatchNodeComponentRemoved(patch.node.get(), patch.component_type); break;
      case ScenePatch::Type::NodeTransformChanged: PatchNodeTransformChanged(patch.node.get()); break;
      default: ZEPHYR_PANIC("Unhandled scene patch type: {}", (int)patch.type);
    }
  }
}

void RenderScene::PatchNodeMounted(SceneNode* node) {
  for(auto& [component_type, _] : node->GetComponents()) {
    PatchNodeComponentMounted(node, component_type);
  }

  PatchNodeTransformChanged(node);
}

void RenderScene::PatchNodeRemoved(SceneNode* node) {
  for(auto& [component_type, _] : node->GetComponents()) {
    PatchNodeComponentRemoved(node, component_type);
  }
}

void RenderScene::PatchNodeComponentMounted(SceneNode* node, std::type_index component_type) {
  if(component_type == typeid(MeshComponent)) {
    const MeshComponent& node_mesh_component = node->GetComponent<MeshComponent>();

    const EntityID entity_id = GetOrCreateEntityForNode(node);
    Mesh& entity_mesh = m_components_mesh[entity_id];
    entity_mesh.geometry = node_mesh_component.geometry.get();
    entity_mesh.material = node_mesh_component.material.get();
    if(entity_mesh.material == nullptr) [[unlikely]] {
      // TODO(fleroviux): this should ideally never happen, but what would be the best way to safeguard against it?
      entity_mesh.material = &m_material_placeholder;
    }
    m_entities[entity_id] |= COMPONENT_FLAG_MESH;
    m_view_mesh.push_back(entity_id);
    m_geometry_cache.IncrementGeometryRefCount(entity_mesh.geometry);
    m_material_cache.IncrementMaterialRefCount(entity_mesh.material);
    m_render_scene_patches.push_back({.type = RenderScenePatch::Type::MeshMounted, .entity_id = entity_id});
  }

  if(component_type == typeid(PerspectiveCameraComponent)) {
    const PerspectiveCameraComponent& node_camera_component = node->GetComponent<PerspectiveCameraComponent>();

    const EntityID entity_id = GetOrCreateEntityForNode(node);
    Camera& entity_camera = m_components_camera[entity_id];
    entity_camera.projection = node_camera_component.GetProjectionMatrix();
    entity_camera.frustum = node_camera_component.GetFrustum();
    m_entities[entity_id] |= COMPONENT_FLAG_CAMERA;
    m_view_camera.push_back(entity_id);
  }
}

void RenderScene::PatchNodeComponentRemoved(SceneNode* node, std::type_index component_type) {
  bool did_remove_component = false;

  if(component_type == typeid(MeshComponent)) {
    const EntityID entity_id = GetOrCreateEntityForNode(node);
    m_entities[entity_id] &= ~COMPONENT_FLAG_MESH;
    m_view_mesh.erase(std::ranges::find(m_view_mesh, entity_id));
    m_geometry_cache.DecrementGeometryRefCount(m_components_mesh[entity_id].geometry);
    m_material_cache.DecrementMaterialRefCount(m_components_mesh[entity_id].material);
    m_render_scene_patches.push_back({.type = RenderScenePatch::Type::MeshRemoved, .entity_id = entity_id});
    did_remove_component = true;
  }

  if(component_type == typeid(PerspectiveCameraComponent)) {
    const EntityID entity_id = GetOrCreateEntityForNode(node);
    m_entities[entity_id] &= ~COMPONENT_FLAG_CAMERA;
    m_view_camera.erase(std::ranges::find(m_view_camera, entity_id));
    did_remove_component = true;
  }

  if(did_remove_component) {
    const EntityID entity_id = m_node_entity_map[node];

    if(m_entities[entity_id] == 0u) {
      DestroyEntity(entity_id);
    }
  }
}

void RenderScene::PatchNodeTransformChanged(SceneNode* node) {
  const auto node_and_entity_id = m_node_entity_map.find(node);
  if(node_and_entity_id == m_node_entity_map.end()) {
    return;
  }

  const EntityID entity_id = node_and_entity_id->second;

  Transform& entity_transform = m_components_transform[entity_id];
  entity_transform.local_to_world = node->GetTransform().GetWorld();
  m_render_scene_patches.push_back({.type = RenderScenePatch::Type::TransformChanged, .entity_id = entity_id});
}

RenderScene::EntityID RenderScene::GetOrCreateEntityForNode(const SceneNode* node) {
  const auto node_and_entity_id = m_node_entity_map.find(node);

  if(node_and_entity_id == m_node_entity_map.end()) {
    const EntityID entity_id = CreateEntity();
    m_node_entity_map[node] = entity_id;
    return entity_id;
  }

  return node_and_entity_id->second;
}

RenderScene::EntityID RenderScene::CreateEntity() {
  if(m_free_entity_list.empty()) {
    m_entities.push_back(0u);
    ResizeComponentStorage(m_entities.size());
    return m_entities.size() - 1;
  }

  const EntityID entity_id = m_free_entity_list.back();
  m_entities[entity_id] = 0u;
  m_free_entity_list.pop_back();
  return entity_id;
}

void RenderScene::DestroyEntity(EntityID entity_id) {
  m_free_entity_list.push_back(entity_id);
}

void RenderScene::ResizeComponentStorage(size_t capacity) {
  m_components_transform.resize(capacity);
  m_components_mesh.resize(capacity);
  m_components_camera.resize(capacity);
}

} // namespace zephyr
