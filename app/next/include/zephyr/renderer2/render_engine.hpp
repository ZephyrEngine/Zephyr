
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <zephyr/renderer2/backend/render_backend.hpp>
#include <zephyr/scene/node.hpp>
#include <atomic>
#include <semaphore>
#include <thread>
#include <vector>

namespace zephyr {

  struct MeshComponent : Component {};

  class RenderEngine {
    public:
      RenderEngine(std::unique_ptr<RenderBackend> render_backend);
     ~RenderEngine();

      void RenderScene(SceneNode* scene_root);

    private:
      void CreateRenderThread();
      void JoinRenderThread();
      void RenderThreadMain();

      std::unique_ptr<RenderBackend> m_render_backend;
      std::thread m_render_thread;
      std::atomic_bool m_render_thread_running;
      std::atomic_bool m_render_thread_is_waiting;
      std::binary_semaphore m_caller_thread_semaphore{0}; //> Semaphore signalled by the calling thread
      std::binary_semaphore m_render_thread_semaphore{1}; //> Semaphore signalled by the rendering thread

      std::vector<RenderObject> m_render_objects;
  };

} // namespace zephyr
