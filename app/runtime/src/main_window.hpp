
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

      std::shared_ptr<RenderDevice> render_device;
      std::shared_ptr<CommandPool> command_pool;
      std::unique_ptr<CommandBuffer> render_command_buffer;
      std::shared_ptr<RenderPass> render_pass;
      std::unique_ptr<Fence> fence;
      std::unique_ptr<GraphicsPipeline> pipeline;
      std::shared_ptr<Buffer> vbo;
      std::shared_ptr<Buffer> ibo;

      int frame = 0;
  };

} // namespace zephyr
