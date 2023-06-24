
#include <zephyr/float.hpp>

#include "main_window.hpp"

namespace zephyr {

  MainWindow::MainWindow() {
    SetWindowSize(1600, 900);
    SetWindowTitle("Zephyr Runtime");
    Setup();
  }

  void MainWindow::OnFrame() {
    const size_t frame_index = m_frame % m_frames_in_flight;
    const auto& command_buffer = m_render_command_buffers[frame_index];
    const auto& fence = m_fences[frame_index];

    if(m_frame >= m_frames_in_flight) { // TODO: set fence create signalled bit?
      fence->Wait();
    }

    const auto& render_target = GetSwapChain()->AcquireNextRenderTarget();

    struct Transform {
      Matrix4 projection;
      Matrix4 model_view;
    } transform;

    transform.projection = m_projection_matrix;

    m_render_pass->SetClearColor(0, 0.02, 0.02, 0.02, 1.0);

    // Do some non-sense calculations to cause load on the CPU side
    f32 jitter = 0.0f;
    for(int i = 0; i < 150000; i++) {
      jitter = std::sin(jitter + (f32)(m_frame + i) * 0.001f);
    }

    const int cubes_per_axis = 25;

    command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
    command_buffer->BeginRenderPass(render_target.get(), m_render_pass.get());
    command_buffer->BindGraphicsPipeline(m_pipeline.get());
    command_buffer->BindVertexBuffers({{m_vbo.get()}});
    command_buffer->BindIndexBuffer(m_ibo.get(), IndexDataType::UInt16);
    for(int z = 0; z < cubes_per_axis; z++) {
      for(int x = 0; x < cubes_per_axis; x++) {
        for(int y = 0; y < cubes_per_axis; y++) {
          const float scene_x = ((f32)x / 25.0f * 2.0f - 1.0f) * 5.0f + jitter * 0.0000001f;
          const float scene_y = ((f32)y / 25.0f * 2.0f - 1.0f) * 5.0f;
          const float scene_z = ((f32)z / 25.0f * 2.0f) * 5.0f + 3.0f;

          transform.model_view = Matrix4::Translation(scene_x, scene_y, -scene_z) *
                                 Matrix4::RotationX((f32)m_frame * 0.025f) *
                                 Matrix4::Scale(0.1f, 0.1f, 0.1f);

          command_buffer->PushConstants(m_pipeline->GetLayout(), 0, sizeof(transform), &transform);
          command_buffer->DrawIndexed(36);
        }
      }
    }
    command_buffer->EndRenderPass();
    command_buffer->End();

    // @todo: remove TmpWaitForImageFullyRead() completely and use a semaphore instead.
    GetSwapChain()->TmpWaitForImageFullyRead();
    fence->Reset();
    m_render_device->GraphicsQueue()->Submit({{command_buffer.get()}}, fence.get());

    // @todo: when is the right time to submit the frame?
    GetSwapChain()->Present();

    m_frame++;

    UpdateFramesPerSecondCounter();
  }

  void MainWindow::OnResize(int width, int height) {
    m_pipeline_builder->SetViewport(0, 0, width, height);
    m_pipeline = m_pipeline_builder->Build();

    m_projection_matrix = Matrix4::PerspectiveVK(45.0f, (f32)width / (f32)height, 0.01f, 100.0f);
  }

  void MainWindow::Setup() {
    m_render_device = GetRenderDevice();

    // @todo: does it make sense to directly couple the frames in flight to the number of swapchain images?
    m_frames_in_flight = GetSwapChain()->GetNumberOfSwapChainImages();
    ZEPHYR_INFO("Renderer configured to have {} frame(s) in flight", m_frames_in_flight);

    CreateCommandPoolAndBuffers();
    CreateRenderPass();
    CreateFences();
    CreateGraphicsPipeline();
    CreateVertexAndIndexBuffer();
  }

  void MainWindow::CreateCommandPoolAndBuffers() {
    m_command_pool = m_render_device->CreateGraphicsCommandPool(
      CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer);

    for(size_t i = 0; i < m_frames_in_flight; i++) {
      m_render_command_buffers.push_back(m_render_device->CreateCommandBuffer(m_command_pool.get()));
    }
  }

  void MainWindow::CreateRenderPass() {
    auto builder = m_render_device->CreateRenderPassBuilder();

    builder->SetColorAttachmentFormat(0, Texture::Format::B8G8R8A8_SRGB);
    builder->SetColorAttachmentSrcLayout(0, Texture::Layout::Undefined, std::nullopt);
    builder->SetColorAttachmentDstLayout(0, Texture::Layout::PresentSrc, std::nullopt);

    builder->SetDepthAttachmentFormat(Texture::Format::DEPTH_U16);
    builder->SetDepthAttachmentSrcLayout(Texture::Layout::Undefined, std::nullopt);
    builder->SetDepthAttachmentDstLayout(Texture::Layout::DepthStencilAttachment, std::nullopt);

    m_render_pass = builder->Build();
  }

  void MainWindow::CreateFences() {
    for(size_t i = 0; i < m_frames_in_flight; i++) {
      m_fences.push_back(m_render_device->CreateFence());
      m_fences[i]->Reset();
    }
  }

  void MainWindow::CreateGraphicsPipeline() {
    std::shared_ptr<ShaderModule> vert_shader = m_render_device->CreateShaderModule(mesh_vert, sizeof(mesh_vert));
    std::shared_ptr<ShaderModule> frag_shader = m_render_device->CreateShaderModule(mesh_frag, sizeof(mesh_frag));

    m_pipeline_builder = m_render_device->CreateGraphicsPipelineBuilder();

    m_pipeline_builder->SetViewport(0, 0, 512, 512);
    m_pipeline_builder->SetShaderModule(PipelineStage::VertexShader, vert_shader);
    m_pipeline_builder->SetShaderModule(PipelineStage::FragmentShader, frag_shader);
    m_pipeline_builder->SetRenderPass(m_render_pass);
    m_pipeline_builder->SetDepthTestEnable(true);
    m_pipeline_builder->SetDepthWriteEnable(true);
    m_pipeline_builder->SetDepthCompareOp(CompareOp::LessOrEqual);
    m_pipeline_builder->AddVertexInputBinding(0, sizeof(float) * 6);
    m_pipeline_builder->AddVertexInputAttribute(0, 0, 0, VertexDataType::Float32, 3, false);
    m_pipeline_builder->AddVertexInputAttribute(1, 0, sizeof(float) * 3, VertexDataType::Float32, 3, false);

    m_pipeline = m_pipeline_builder->Build();
  }

  void MainWindow::CreateVertexAndIndexBuffer() {
    /**
     *   4-------5
     *  /|      /|
     * 0-------1 |
     * | 6-----|-7
     * |/      |/
     * 2-------3
     */
    static const float k_vertices[] = {
      //    | POSITION       | COLOR         |

      // front face
      /*0*/ -1.0, -1.0,  1.0,  1.0, 0.0, 0.0,
      /*1*/  1.0, -1.0,  1.0,  0.0, 1.0, 0.0,
      /*2*/ -1.0,  1.0,  1.0,  0.0, 0.0, 1.0,
      /*3*/  1.0,  1.0,  1.0,  0.5, 0.5, 0.5,

      // back face
      /*4*/ -1.0, -1.0, -1.0,  1.0, 1.0, 0.0,
      /*5*/  1.0, -1.0, -1.0,  1.0, 0.0, 1.0,
      /*6*/ -1.0,  1.0, -1.0,  0.0, 1.0, 1.0,
      /*7*/  1.0,  1.0, -1.0,  1.0, 1.0, 1.0
    };

    static const u16 k_indices[] = {
      // front
      0, 1, 2,
      1, 3, 2,

      // back
      4, 5, 6,
      5, 7, 6,

      // left
      0, 4, 6,
      0, 6, 2,

      // right
      1, 5, 7,
      1, 7, 3,

      // top
      4, 1, 0,
      4, 5, 1,

      // bottom
      6, 3, 2,
      6, 7, 3
    };

    auto staging_vbo = m_render_device->CreateBuffer(
      Buffer::Usage::CopySrc, Buffer::Flags::HostVisible, sizeof(k_vertices));

    auto staging_ibo = m_render_device->CreateBuffer(
      Buffer::Usage::CopySrc, Buffer::Flags::HostVisible, sizeof(k_indices));

    m_vbo = m_render_device->CreateBuffer(
      Buffer::Usage::VertexBuffer | Buffer::Usage::CopyDst, Buffer::Flags::None, sizeof(k_vertices));

    m_ibo = m_render_device->CreateBuffer(
      Buffer::Usage::IndexBuffer | Buffer::Usage::CopyDst, Buffer::Flags::None, sizeof(k_indices));

    // @todo: fix the API so that we either have to Map() the buffer, or don't have to unmap it.
    staging_vbo->Update<u8>((u8 const*)k_vertices, sizeof(k_vertices));
    staging_vbo->Unmap();

    staging_ibo->Update<u8>((u8 const*)k_indices, sizeof(k_indices));
    staging_ibo->Unmap();

    auto command_buffer = m_render_device->CreateCommandBuffer(m_command_pool.get());

    command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
    command_buffer->CopyBuffer(staging_vbo.get(), m_vbo.get(), m_vbo->Size());
    command_buffer->CopyBuffer(staging_ibo.get(), m_ibo.get(), m_ibo->Size());
    command_buffer->End();

    m_render_device->GraphicsQueue()->Submit({{command_buffer.get()}});
    m_render_device->GraphicsQueue()->WaitIdle();
  }

  void MainWindow::UpdateFramesPerSecondCounter() {
    const auto time_point_now = std::chrono::steady_clock::now();

    const auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      time_point_now - m_time_point_last_update).count();

    m_fps_counter++;

    if(time_elapsed >= 1000) {
      const f32 fps = (f32)m_fps_counter * 1000.0f / (f32)time_elapsed;
      SetWindowTitle(fmt::format("Zephyr Runtime ({} fps)", fps));
      m_fps_counter = 0;
      m_time_point_last_update = std::chrono::steady_clock::now();
    }
  }

} // namespace zephyr
