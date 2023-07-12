
#pragma once

#include <zephyr/logger/logger.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/renderer/buffer/index_buffer.hpp>
#include <zephyr/renderer/buffer/uniform_buffer.hpp>
#include <zephyr/renderer/buffer/vertex_buffer.hpp>
#include <zephyr/renderer/texture/texture_2D.hpp>
#include <zephyr/renderer/texture/texture_cube.hpp>
#include <zephyr/window/window.hpp>
#include <chrono>
#include <vector>

#include <fmt/format.h>
#include <zephyr/integer.hpp>

#include "renderer/buffer_cache.hpp"
#include "renderer/material_pipeline_cache.hpp"
#include "renderer/resource_uploader.hpp"
#include "renderer/sampler_cache.hpp"
#include "renderer/texture_cache.hpp"

#include "shader/mesh.vert.h"
#include "shader/mesh.frag.h"
#include "shader/rasterize.comp.h"

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
      void CreateFences();
      void CreateBindGroups();
      void CreateComputePipeline();
      void CreateVertexAndIndexSSBOs();
      void CreateUBO();
      void UpdateFramesPerSecondCounter();

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<CommandPool> m_command_pool;
      std::vector<std::unique_ptr<CommandBuffer>> m_render_command_buffers;
      std::shared_ptr<RenderPass> m_render_pass;
      std::vector<std::unique_ptr<Fence>> m_fences;
      std::shared_ptr<ResourceUploader> m_resource_uploader;
      std::shared_ptr<BufferCache> m_buffer_cache;
      std::shared_ptr<TextureCache> m_texture_cache;
      std::shared_ptr<BindGroupLayout> m_bind_group_layout;
      std::vector<std::unique_ptr<BindGroup>> m_bind_groups;
      std::unique_ptr<ComputePipeline> m_compute_pipeline;
      std::unique_ptr<BufferResource> m_index_ssbo;
      std::unique_ptr<BufferResource> m_vertex_ssbo;
      std::unique_ptr<BufferResource> m_ubo;

      uint m_frame{0};
      uint m_frames_in_flight{};

      int m_fps_counter{};
      std::chrono::steady_clock::time_point m_time_point_last_update;

      int m_width{};
      int m_height{};
  };

} // namespace zephyr
