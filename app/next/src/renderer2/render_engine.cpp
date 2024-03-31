
#include <zephyr/renderer2/render_engine.hpp>
#include <fmt/format.h>

namespace zephyr {

  RenderEngine::RenderEngine(std::unique_ptr<RenderBackend> render_backend) : m_render_backend{std::move(render_backend)} {
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
          .local_to_world = node->GetTransform().GetWorld()
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

      // TODO(fleroviux): do not hardcode the aspect ratio.
      const Matrix4 projection = Matrix4::PerspectiveVK(45.0f, 16.0f/9.0, 0.01f, 100.0f);
      m_render_backend->Render(projection, m_render_objects);
      m_render_backend->SwapBuffers();

      // Signal to the caller thread that we are done reading the internal render structures.
      m_render_thread_semaphore.release();
    }
  }

} // namespace zephyr