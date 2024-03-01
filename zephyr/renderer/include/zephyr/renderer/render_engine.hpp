
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <zephyr/scene/node.hpp>
#include <zephyr/scene/mesh.hpp>
#include <atomic>
#include <semaphore>
#include <thread>
#include <vector>

namespace zephyr {

  struct RenderObject {
    Matrix4 local_to_world_transform;
  };

  class RenderEngine {
    public:
      RenderEngine();
     ~RenderEngine();

      void RenderScene(SceneNode* scene_root);

    private:
      void CreateRenderThread();
      void JoinRenderThread();
      void RenderThreadMain();

      std::thread m_render_thread;
      std::atomic_bool m_render_thread_running;
      std::atomic_bool m_render_thread_is_waiting;
      std::binary_semaphore m_caller_thread_semaphore{0}; //> Semaphore signalled by the calling thread
      std::binary_semaphore m_render_thread_semaphore{1}; //> Semaphore signalled by the rendering thread

      std::vector<RenderObject> m_render_objects;
  };

} // namespace zephyr
