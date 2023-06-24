
#pragma once

#include <zephyr/logger/logger.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/window/window.hpp>

#include <fmt/format.h>
#include <zephyr/integer.hpp>

#include "shader/mesh.vert.h"
#include "shader/mesh.frag.h"

namespace zephyr {

  class MainWindow final : public Window {
    public:
      MainWindow();

      void OnFrame() override;

    private:

      void Setup();
      void CreateCommandPoolAndBuffer();
      void CreateRenderPass();
      void CreateFence();
      void CreateGraphicsPipeline();
      void CreateVertexAndIndexBuffer();

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<CommandPool> m_command_pool;
      std::unique_ptr<CommandBuffer> m_render_command_buffer;
      std::shared_ptr<RenderPass> m_render_pass;
      std::unique_ptr<Fence> m_fence;
      std::unique_ptr<GraphicsPipeline> m_pipeline;
      std::shared_ptr<Buffer> m_vbo;
      std::shared_ptr<Buffer> m_ibo;

      int m_frame{0};
  };

} // namespace zephyr
