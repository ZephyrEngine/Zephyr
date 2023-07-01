
#include <zephyr/float.hpp>

#include "main_window.hpp"

namespace zephyr {

  MainWindow::MainWindow() {
    SetWindowSize(1600, 900);
    SetWindowTitle("Zephyr Runtime");
    Setup();
  }

  MainWindow::~MainWindow() {
    m_render_device->WaitIdle();
  }

  void MainWindow::OnFrame() {
    const uint frame_index = m_frame % m_frames_in_flight;
    const auto& command_buffer = m_render_command_buffers[frame_index];
    const auto& fence = m_fences[frame_index];
    const auto& bind_group = m_bind_groups[frame_index];

    fence->Wait();

    m_resource_uploader->BeginFrame();

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

#ifdef __APPLE__
    const int cubes_per_axis = 5;
#else
    const int cubes_per_axis = 25;
#endif

    m_vbo->Write(0u, -1.0f - 0.1f + std::sin((f32)m_frame * 0.05f), 0u);
    m_vbo->MarkAsDirty();

    Buffer* vbo = m_buffer_cache->GetDeviceBuffer(m_vbo.get());
    Buffer* ibo = m_buffer_cache->GetDeviceBuffer(m_ibo.get());

    bind_group->Bind(0u, m_buffer_cache->GetDeviceBuffer(m_ubo.get()), BindingType::UniformBuffer);

    // Texture streaming test
    // @todo: make the render device API for this not suck
    {
      Buffer* const staging_buffer = m_resource_uploader->GetCurrentStagingBuffer();

      const size_t texture_data_size = m_texture->GetWidth() * m_texture->GetHeight() * sizeof(u32);

      const size_t staging_buffer_offset = m_resource_uploader->AllocateStagingMemory(texture_data_size);

      std::memcpy((u8*)staging_buffer->Data() + staging_buffer_offset, m_texture_data, texture_data_size);

      CommandBuffer* const resource_command_buffer = m_resource_uploader->GetCurrentCommandBuffer();

      resource_command_buffer->PipelineBarrier(
        PipelineStage::TopOfPipe,
        PipelineStage::Transfer, {{
          {
            m_texture.get(),
            Access::None,
            Access::TransferWrite,
            Texture::Layout::Undefined,
            Texture::Layout::CopyDst,
            m_texture->DefaultSubresourceRange()
          }
      }});

      resource_command_buffer->CopyBufferToTexture(
        staging_buffer, staging_buffer_offset, m_texture.get(), Texture::Layout::CopyDst);

      resource_command_buffer->PipelineBarrier(
        PipelineStage::Transfer,
        PipelineStage::VertexShader, {{
          {
            m_texture.get(),
            Access::TransferWrite,
            Access::ShaderRead,
            Texture::Layout::CopyDst,
            Texture::Layout::ShaderReadOnly,
            m_texture->DefaultSubresourceRange()
          }
      }});

      bind_group->Bind(1u, m_texture.get(), m_render_device->DefaultLinearSampler(), Texture::Layout::ShaderReadOnly);
    }

    command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
    command_buffer->SetViewport(0, 0, m_width, m_height);
    command_buffer->BeginRenderPass(render_target.get(), m_render_pass.get());
    command_buffer->BindPipeline(m_pipeline.get());
    command_buffer->BindVertexBuffers({{vbo}});
    command_buffer->BindIndexBuffer(ibo, m_ibo->GetDataType());
    command_buffer->BindBindGroup(PipelineBindPoint::Graphics, m_pipeline->GetLayout(), 0, bind_group.get());
    for(int z = 0; z < cubes_per_axis; z++) {
      for(int x = 0; x < cubes_per_axis; x++) {
        for(int y = 0; y < cubes_per_axis; y++) {
          const float scene_x = ((f32)x / (f32)cubes_per_axis * 2.0f - 1.0f) * 5.0f + jitter * 0.0000001f;
          const float scene_y = ((f32)y / (f32)cubes_per_axis * 2.0f - 1.0f) * 5.0f;
          const float scene_z = ((f32)z / (f32)cubes_per_axis * 2.0f) * 5.0f + 3.0f;

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

    m_resource_uploader->FinalizeFrame();

    // @todo: remove TmpWaitForImageFullyRead() completely and use a semaphore instead.
    GetSwapChain()->TmpWaitForImageFullyRead();
    fence->Reset();
    m_render_device->GraphicsQueue()->Submit({{m_resource_uploader->GetCurrentCommandBuffer(), command_buffer.get()}}, fence.get());

    // @todo: when is the right time to submit the frame?
    GetSwapChain()->Present();

    m_frame++;

    UpdateFramesPerSecondCounter();
  }

  void MainWindow::OnResize(int width, int height) {
    m_width = width;
    m_height = height;
    m_projection_matrix = Matrix4::PerspectiveVK(45.0f, (f32)width / (f32)height, 0.01f, 100.0f);
  }

  void MainWindow::Setup() {
    m_render_device = GetRenderDevice();

    // @todo: does it make sense to directly couple the frames in flight to the number of swapchain images?
    m_frames_in_flight = GetSwapChain()->GetNumberOfSwapChainImages();
    ZEPHYR_INFO("Renderer configured to have {} frame(s) in flight", m_frames_in_flight);

    CreateCommandPoolAndBuffers();
    CreateResourceUploader();
    CreateBufferCache();
    CreateRenderPass();
    CreateFences();
    CreateBindGroups();
    CreateGraphicsPipeline();
    CreateVertexAndIndexBuffer();
    CreateUniformBuffer();
    CreateTexture();
  }

  void MainWindow::CreateCommandPoolAndBuffers() {
    m_command_pool = m_render_device->CreateGraphicsCommandPool(
      CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer);

    for(uint i = 0; i < m_frames_in_flight; i++) {
      m_render_command_buffers.push_back(m_render_device->CreateCommandBuffer(m_command_pool));
    }
  }

  void MainWindow::CreateResourceUploader() {
    m_resource_uploader = std::make_shared<ResourceUploader>(m_render_device, m_command_pool, m_frames_in_flight);
  }

  void MainWindow::CreateBufferCache() {
    m_buffer_cache = std::make_shared<BufferCache>(m_render_device, m_resource_uploader);
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
    for(uint i = 0; i < m_frames_in_flight; i++) {
      m_fences.push_back(m_render_device->CreateFence(Fence::CreateSignalled::Yes));
    }
  }

  void MainWindow::CreateBindGroups() {
    m_bind_group_layout = m_render_device->CreateBindGroupLayout({{
      {
        .binding = 0u,
        .type = BindingType::UniformBuffer,
        .stages = ShaderStage::All
      },
      {
        .binding = 1u,
        .type = BindingType::ImageWithSampler,
        .stages = ShaderStage::All
      },
    }});

    for(uint i = 0; i < m_frames_in_flight; i++) {
      m_bind_groups.push_back(m_render_device->CreateBindGroup(m_bind_group_layout));
    }
  }

  void MainWindow::CreateGraphicsPipeline() {
    std::shared_ptr<ShaderModule> vert_shader = m_render_device->CreateShaderModule(mesh_vert, sizeof(mesh_vert));
    std::shared_ptr<ShaderModule> frag_shader = m_render_device->CreateShaderModule(mesh_frag, sizeof(mesh_frag));

    m_pipeline_builder = m_render_device->CreateGraphicsPipelineBuilder();

    m_pipeline_builder->SetDynamicViewportEnable(true);
    m_pipeline_builder->SetShaderModule(ShaderStage::Vertex, vert_shader);
    m_pipeline_builder->SetShaderModule(ShaderStage::Fragment, frag_shader);
    m_pipeline_builder->SetRenderPass(m_render_pass);
    m_pipeline_builder->SetDepthTestEnable(true);
    m_pipeline_builder->SetDepthWriteEnable(true);
    m_pipeline_builder->SetDepthCompareOp(CompareOp::LessOrEqual);
    m_pipeline_builder->AddVertexInputBinding(0, sizeof(float) * 6);
    m_pipeline_builder->AddVertexInputAttribute(0, 0, 0, VertexDataType::Float32, 3, false);
    m_pipeline_builder->AddVertexInputAttribute(1, 0, sizeof(float) * 3, VertexDataType::Float32, 3, false);
    m_pipeline_builder->SetPipelineLayout(m_render_device->CreatePipelineLayout({{m_bind_group_layout.get()}}));

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

    m_vbo = std::make_unique<VertexBuffer>(6 * sizeof(float), std::span{(const u8*)k_vertices, sizeof(k_vertices)});
    m_ibo = std::make_unique<IndexBuffer>(IndexDataType::UInt16, std::span{(const u8*)k_indices, sizeof(k_indices)});
  }

  void MainWindow::CreateUniformBuffer() {
    Vector4 color{1.0f, 0.0f, 0.0f, 0.0f};

    m_ubo = std::make_unique<UniformBuffer>(std::span{(const u8*)&color, sizeof(color)});
  }

  void MainWindow::CreateTexture() {
    m_texture = m_render_device->CreateTexture2D(
      512, 512, Texture::Format::R8G8B8A8_SRGB, Texture::Usage::CopyDst | Texture::Usage::Sampled);

    m_texture_data = new u32[512 * 512];

    for(int y = 0; y < 512; y++) {
      for(int x = 0; x < 512; x++) {
        m_texture_data[y * 512 + x] = 0xFFFF00FF;
      }
    }
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
