
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/render_scene.hpp>
#include <zephyr/scene/scene_graph.hpp>
#include <atomic>
#include <semaphore>
#include <thread>

namespace zephyr {

  class RenderEngine {
    public:
      explicit RenderEngine(std::unique_ptr<RenderBackend> render_backend);
     ~RenderEngine();

      void SetSceneGraph(std::shared_ptr<SceneGraph> scene_graph);
      void SubmitFrame();

    private:
      void CreateRenderThread();
      void JoinRenderThread();
      void RenderThreadMain();
      void ReadyRenderThreadData();

      std::shared_ptr<RenderBackend> m_render_backend;

      std::thread m_render_thread;
      std::atomic_bool m_render_thread_running;
      std::atomic_bool m_render_thread_is_waiting;
      std::binary_semaphore m_caller_thread_semaphore{0}; //< Semaphore signalled by the calling thread
      std::binary_semaphore m_render_thread_semaphore{1}; //< Semaphore signalled by the rendering thread

      RenderScene m_render_scene; //< Representation of the scene graph that is internal to the render engine.
      RenderCamera m_render_camera{};
  };

} // namespace zephyr
