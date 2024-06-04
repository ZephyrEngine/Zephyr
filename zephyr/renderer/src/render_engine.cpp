
#include <zephyr/renderer/component/camera.hpp>
#include <zephyr/renderer/component/mesh.hpp>
#include <zephyr/renderer/render_engine.hpp>
#include <fmt/format.h>

namespace zephyr {

  RenderEngine::RenderEngine(std::unique_ptr<RenderBackend> render_backend)
      : m_render_backend{std::move(render_backend)}
      , m_geometry_cache{m_render_backend} {
    CreateRenderThread();
  }

  RenderEngine::~RenderEngine() {
    JoinRenderThread();
  }

  void RenderEngine::SetSceneGraph(std::shared_ptr<SceneGraph> scene_graph) {
    if(m_current_scene_graph != scene_graph) {
      m_current_scene_graph = std::move(scene_graph);
      m_need_scene_rebuild = true;
    }
  }

  void RenderEngine::RenderScene() {
    // Wait for the render thread to complete reading the internal render structures.
    m_render_thread_semaphore.acquire();

    // Instruct the geometry cache to evict geometries which had been deleted in the submitted frame.
    m_geometry_cache.CommitPendingDeleteTaskList();

    // Update the internal scene graph representation of the render engine based on changes in the scene graph.
    if(m_need_scene_rebuild) {
      RebuildScene();
      m_need_scene_rebuild = false;
    } else {
      PatchScene();
    }

    // Update all geometries which might be rendered in this frame.
    for(const auto& game_thread_render_object : m_scene_node_mesh_data) {
      m_geometry_cache.UpdateGeometry(game_thread_render_object.geometry);
    }

    // Signal to the render thread that the next frame is ready
    m_caller_thread_semaphore.release();
  }

  void RenderEngine::RebuildScene() {
    m_scene_node_data.clear();
    m_scene_node_mesh_data.clear();
    m_scene_node_camera_data.clear();

    m_current_scene_graph->GetRoot()->Traverse([this](SceneNode* child_node) {
      if(child_node->IsVisible()) {
        PatchNodeMounted(child_node);
        return true;
      }
      return false;
    });
  }

  void RenderEngine::PatchScene() {
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

  void RenderEngine::PatchNodeMounted(SceneNode* node) {
    for(auto& [component_type, _] : node->GetComponents()) {
      PatchNodeComponentMounted(node, component_type);
    }
  }

  void RenderEngine::PatchNodeRemoved(SceneNode* node) {
    // TODO(fleroviux): this could be optimized, just unload everything.
    for(auto& [component_type, _] : node->GetComponents()) {
      PatchNodeComponentRemoved(node, component_type);
    }
  }

  void RenderEngine::PatchNodeComponentMounted(SceneNode* node, std::type_index component_type) {
    if(component_type == typeid(MeshComponent)) {
      SceneNodeData& node_data = m_scene_node_data[node];
      auto& mesh_component = node->GetComponent<MeshComponent>();

      node_data.mesh_data_id = m_scene_node_mesh_data.size();
      m_scene_node_mesh_data.emplace_back(node->GetTransform().GetWorld(), mesh_component.geometry.get());
    }

    if(component_type == typeid(PerspectiveCameraComponent)) {
      SceneNodeData& node_data = m_scene_node_data[node];
      auto& camera_component = node->GetComponent<PerspectiveCameraComponent>();

      node_data.camera_data_id = m_scene_node_camera_data.size();
      m_scene_node_camera_data.push_back({
        .projection = camera_component.GetProjectionMatrix(),
        .view = node->GetTransform().GetWorld().Inverse(),
        .frustum = camera_component.GetFrustum()
      });
    }
  }

  void RenderEngine::PatchNodeComponentRemoved(SceneNode* node, std::type_index component_type) {
    if(!m_scene_node_data.contains(node)) {
      return;
    }

    if(component_type == typeid(MeshComponent)) {
      SceneNodeData& node_data = m_scene_node_data[node];
      if(node_data.mesh_data_id.has_value()) {
        // TODO(fleroviux): this breaks indices in other SceneNodeData
        m_scene_node_mesh_data.erase(m_scene_node_mesh_data.begin() + node_data.mesh_data_id.value());
        node_data.mesh_data_id.reset();
      }
    }

    if(component_type == typeid(PerspectiveCameraComponent)) {
      SceneNodeData& node_data = m_scene_node_data[node];
      if(node_data.camera_data_id.has_value()) {
        // TODO(fleroviux): this breaks indices in other SceneNodeData
        m_scene_node_camera_data.erase(m_scene_node_camera_data.begin() + node_data.camera_data_id.value());
        node_data.camera_data_id.reset();
      }
    }

    if(m_scene_node_data[node].Empty()) {
      m_scene_node_data.erase(node);
    }
  }

  void RenderEngine::PatchNodeTransformChanged(SceneNode* node) {
    if(!m_scene_node_data.contains(node)) {
      return;
    }

    const SceneNodeData& node_data = m_scene_node_data[node];

    if(node_data.mesh_data_id.has_value()) {
      m_scene_node_mesh_data[node_data.mesh_data_id.value()].local_to_world = node->GetTransform().GetWorld();
    }

    if(node_data.camera_data_id.has_value()) {
      m_scene_node_camera_data[node_data.camera_data_id.value()].view = node->GetTransform().GetWorld().Inverse();
    }
  }

  void RenderEngine::CreateRenderThread() {
    m_render_thread_running = true;
    m_render_thread_is_waiting = false;
    m_render_thread = std::thread{[this] { RenderThreadMain(); }};
  }

  void RenderEngine::JoinRenderThread() {
    m_render_thread_running = false;
    if(m_render_thread_is_waiting) {
      m_caller_thread_semaphore.release();
    }
    m_render_thread.join();
  }

  void RenderEngine::RenderThreadMain() {
    m_render_backend->InitializeContext();

    while(m_render_thread_running) {
      ReadyRenderThreadData();

      m_render_backend->Render(m_render_camera, m_render_objects);
      m_render_backend->SwapBuffers();
    }

    m_render_backend->DestroyContext();
  }

  void RenderEngine::ReadyRenderThreadData() {
    // Wait for the caller thread to prepare the internal render structures for the next frame.
    m_render_thread_is_waiting = true;
    m_caller_thread_semaphore.acquire();
    m_render_thread_is_waiting = false;

    m_geometry_cache.ProcessPendingUpdates();

    m_render_objects.clear();

    for(const auto& scene_node_mesh_data : m_scene_node_mesh_data) {
      m_render_objects.push_back({
        .render_geometry = m_geometry_cache.GetCachedRenderGeometry(scene_node_mesh_data.geometry),
        .local_to_world = scene_node_mesh_data.local_to_world
      });
    }

    if(m_scene_node_camera_data.empty()) {
      ZEPHYR_PANIC("Scene graph does not contain a camera to render with.");
    }
    m_render_camera = m_scene_node_camera_data[0];

    // Signal to the caller thread that we are done reading the internal render structures.
    m_render_thread_semaphore.release();
  }

} // namespace zephyr