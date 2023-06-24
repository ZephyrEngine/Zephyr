
#pragma once

#include <zephyr/logger/logger.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/window/window.hpp>
#include <chrono>
#include <vector>

#include <fmt/format.h>
#include <zephyr/integer.hpp>

#include "shader/mesh.vert.h"
#include "shader/mesh.frag.h"

namespace zephyr {

  class MainWindow final : public Window {
    public:
      MainWindow();

      void OnFrame() override;
      void OnResize(int width, int height) override;

    private:

      void Setup();
      void CreateCommandPoolAndBuffers();
      void CreateRenderPass();
      void CreateFences();
      void CreateGraphicsPipeline();
      void CreateVertexAndIndexBuffer();
      void UpdateFramesPerSecondCounter();

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<CommandPool> m_command_pool;
      std::vector<std::unique_ptr<CommandBuffer>> m_render_command_buffers;
      std::shared_ptr<RenderPass> m_render_pass;
      std::vector<std::unique_ptr<Fence>> m_fences;
      std::unique_ptr<GraphicsPipelineBuilder> m_pipeline_builder;
      std::unique_ptr<GraphicsPipeline> m_pipeline;
      std::shared_ptr<Buffer> m_vbo;
      std::shared_ptr<Buffer> m_ibo;

      Matrix4 m_projection_matrix;
      int m_frame{0};
      size_t m_frames_in_flight{};

      int m_fps_counter{};
      std::chrono::steady_clock::time_point m_time_point_last_update;
  };

} // namespace zephyr
