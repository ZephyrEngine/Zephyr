
#include <zephyr/renderer/render_engine.hpp>
#include <fmt/format.h>

namespace zephyr {

  RenderEngine::RenderEngine() {
    CreateRenderThread();
  }

  RenderEngine::~RenderEngine() {
    JoinRenderThread();
  }

  void RenderEngine::RenderScene(SceneNode* scene_root) {
    // Wait for the render thread to complete reading the internal render structures.
    m_render_thread_semaphore.acquire();

    m_render_objects.clear();

    scene_root->Traverse([&](SceneNode* node) -> bool {
      if(!node->IsVisible()) return false;

      if(node->HasComponent<MeshComponent>()) {
        m_render_objects.push_back({
          .local_to_world_transform = node->GetTransform().GetWorld()
        });
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
    while(m_render_thread_running) {
      // Wait for the caller thread to prepare the internal render structures for the next frame.
      m_render_thread_is_waiting = true;
      m_caller_thread_semaphore.acquire();
      m_render_thread_is_waiting = false;

      fmt::print("got {} render objects\n", m_render_objects.size());

      // Signal to the caller thread that we are done reading the internal render structures.
      m_render_thread_semaphore.release();
    }
  }

} // namespace zephyr