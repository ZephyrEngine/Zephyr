
#include <zephyr/renderer/render_engine.hpp>

namespace zephyr {

  RenderEngine::RenderEngine(std::unique_ptr<RenderBackend> render_backend)
      : m_render_backend{std::move(render_backend)}
      , m_render_scene{m_render_backend} {
    CreateRenderThread();
  }

  RenderEngine::~RenderEngine() {
    JoinRenderThread();
  }

  void RenderEngine::SetSceneGraph(std::shared_ptr<SceneGraph> scene_graph) {
    m_render_scene.SetSceneGraph(std::move(scene_graph));
  }

  void RenderEngine::SubmitFrame() {
    // Wait for the render thread to complete reading the internal render structures.
    m_render_thread_semaphore.acquire();

    // Update the GPU scene based on changes in the scene graph (stage 1)
    m_render_scene.UpdateStage1();

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

      // m_render_backend->Render(m_render_camera, m_render_objects);
      m_render_backend->Render(m_render_camera, m_render_scene.GetRenderBundles());
      m_render_backend->SwapBuffers();
    }

    m_render_backend->DestroyContext();
  }

  void RenderEngine::ReadyRenderThreadData() {
    // Wait for the caller thread to prepare the internal render structures for the next frame.
    m_render_thread_is_waiting = true;
    m_caller_thread_semaphore.acquire();
    m_render_thread_is_waiting = false;

    // Update the GPU scene based on changes in the scene graph (stage 2)
    m_render_scene.UpdateStage2();

    m_render_scene.GetRenderCamera(m_render_camera);

    // Signal to the caller thread that we are done reading the internal render structures.
    m_render_thread_semaphore.release();
  }

} // namespace zephyr