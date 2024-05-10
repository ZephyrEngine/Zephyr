
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

  void RenderEngine::RenderScene(SceneNode* scene_root) {
    // Wait for the render thread to complete reading the internal render structures.
    m_render_thread_semaphore.acquire();

    // Instruct the geometry cache to evict geometries which had been deleted in the submitted frame.
    m_geometry_cache.CommitPendingDeleteTaskList();

    // Traverse the scene and update any data structures (such as render lists and resource caches) needed to render the frame.
    m_game_thread_render_objects.clear();
    m_render_camera[1] = {};
    scene_root->Traverse([&](SceneNode* node) -> bool {
      if(!node->IsVisible()) return false;

      if(node->HasComponent<MeshComponent>()) {
        const MeshComponent& mesh_component = node->GetComponent<MeshComponent>();
        const auto& geometry = mesh_component.geometry;

        if(geometry) {
          m_geometry_cache.UpdateGeometry(geometry.get());

          m_game_thread_render_objects.push_back({
            .local_to_world = node->GetTransform().GetWorld(),
            .geometry = geometry.get()
          });
        }
      }

      // TODO(fleroviux): think of a better way to mark the camera we actually want to use.
      if(node->HasComponent<PerspectiveCameraComponent>()) {
        const PerspectiveCameraComponent& camera_component = node->GetComponent<PerspectiveCameraComponent>();
        m_render_camera[1].projection = camera_component.GetProjectionMatrix();
        m_render_camera[1].view = node->GetTransform().GetWorld().Inverse();
      }

      return true;
    });

    // Signal to the render thread that the next frame is ready
    m_caller_thread_semaphore.release();
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

      m_render_backend->Render(m_render_camera[0], m_render_objects);
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

    for(const auto& game_thread_render_object : m_game_thread_render_objects) {
      m_render_objects.push_back({
        .render_geometry = m_geometry_cache.GetCachedRenderGeometry(game_thread_render_object.geometry),
        .local_to_world = game_thread_render_object.local_to_world
      });
    }

    m_render_camera[0] = m_render_camera[1];

    // Signal to the caller thread that we are done reading the internal render structures.
    m_render_thread_semaphore.release();
  }

} // namespace zephyr