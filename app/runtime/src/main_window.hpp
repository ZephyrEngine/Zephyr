
#pragma once

#include <zephyr/logger/logger.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/renderer/buffer/index_buffer.hpp>
#include <zephyr/renderer/buffer/uniform_buffer.hpp>
#include <zephyr/renderer/buffer/vertex_buffer.hpp>
#include <zephyr/renderer/texture/texture_2D.hpp>
#include <zephyr/window/window.hpp>
#include <chrono>
#include <vector>

#include <fmt/format.h>
#include <zephyr/integer.hpp>

#include "renderer/buffer_cache.hpp"
#include "renderer/resource_uploader.hpp"
#include "renderer/sampler_cache.hpp"
#include "renderer/texture_cache.hpp"

#include "shader/mesh.vert.h"
#include "shader/mesh.frag.h"

namespace zephyr {

  class MainWindow final : public Window {
    public:
      MainWindow();
     ~MainWindow() override;

      void OnFrame() override;
      void OnResize(int width, int height) override;

    private:

      void Setup();
      void CreateCommandPoolAndBuffers();
      void CreateResourceUploader();
      void CreateBufferCache();
      void CreateTextureCache();
      void CreateSamplerCache();
      void CreateRenderPass();
      void CreateFences();
      void CreateGraphicsPipeline();
      void CreateVertexAndIndexBuffer();
      void CreateUniformBuffer();
      void CreateBindGroups();
      void CreateTexture();
      void UpdateFramesPerSecondCounter();

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<CommandPool> m_command_pool;
      std::vector<std::unique_ptr<CommandBuffer>> m_render_command_buffers;
      std::shared_ptr<RenderPass> m_render_pass;
      std::vector<std::unique_ptr<Fence>> m_fences;
      std::unique_ptr<GraphicsPipelineBuilder> m_pipeline_builder;
      std::unique_ptr<GraphicsPipeline> m_pipeline;
      std::unique_ptr<VertexBuffer> m_vbo;
      std::unique_ptr<IndexBuffer> m_ibo;
      std::unique_ptr<UniformBuffer> m_ubo;
      std::shared_ptr<ResourceUploader> m_resource_uploader;
      std::shared_ptr<BufferCache> m_buffer_cache;
      std::shared_ptr<TextureCache> m_texture_cache;
      std::shared_ptr<SamplerCache> m_sampler_cache;
      std::shared_ptr<BindGroupLayout> m_bind_group_layout;
      std::vector<std::unique_ptr<BindGroup>> m_bind_groups;

      std::unique_ptr<Texture2D> m_texture;

      Matrix4 m_projection_matrix;
      uint m_frame{0};
      uint m_frames_in_flight{};

      int m_fps_counter{};
      std::chrono::steady_clock::time_point m_time_point_last_update;

      int m_width{};
      int m_height{};
  };

} // namespace zephyr
