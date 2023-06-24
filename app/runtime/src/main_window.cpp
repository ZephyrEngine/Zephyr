
#include <zephyr/float.hpp>

#include "main_window.hpp"

namespace zephyr {

  MainWindow::MainWindow() {
    SetWindowSize(1600, 900);
    SetWindowTitle("Zephyr Runtime");
    Setup();
  }

  void MainWindow::OnFrame() {
    const auto& render_target = GetSwapChain()->AcquireNextRenderTarget();

    struct Transform {
      Matrix4 projection;
      Matrix4 model_view;
    } transform;

    transform.projection = m_projection_matrix;

    m_render_pass->SetClearColor(0, 0.02, 0.02, 0.02, 1.0);

    m_render_command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
    m_render_command_buffer->BeginRenderPass(render_target.get(), m_render_pass.get());
    m_render_command_buffer->BindGraphicsPipeline(m_pipeline.get());
    m_render_command_buffer->BindVertexBuffers({{m_vbo.get()}});
    m_render_command_buffer->BindIndexBuffer(m_ibo.get(), IndexDataType::UInt16);
    for(int x = 0; x < 25; x++) {
      for(int y = 0; y < 25; y++) {
        for(int z = 24; z >= 0; z--) {
          const float scene_x = ((f32)x / 25.0f * 2.0f - 1.0f) * 5.0f;
          const float scene_y = ((f32)y / 25.0f * 2.0f - 1.0f) * 5.0f;
          const float scene_z = ((f32)z / 25.0f * 2.0f) * 5.0f + 3.0f;

          transform.model_view = Matrix4::Translation(scene_x, scene_y, -scene_z) *
                                 Matrix4::RotationX((f32)m_frame * 0.025f) *
                                 Matrix4::Scale(0.1f, 0.1f, 0.1f);

          m_render_command_buffer->PushConstants(m_pipeline->GetLayout(), 0, sizeof(transform), &transform);
          m_render_command_buffer->DrawIndexed(36);
        }
      }
    }
    m_render_command_buffer->EndRenderPass();
    m_render_command_buffer->End();

    m_fence->Reset();
    m_render_device->GraphicsQueue()->Submit({{m_render_command_buffer.get()}}, m_fence.get());
    m_fence->Wait();

    GetSwapChain()->Present();

    m_frame++;
  }

  void MainWindow::OnResize(int width, int height) {
    // @todo: should we wait for all frames in flight to be processed or is this safe to do?
    m_pipeline_builder->SetViewport(0, 0, width, height);
    m_pipeline = m_pipeline_builder->Build();

    m_projection_matrix = Matrix4::PerspectiveVK(45.0f, (f32)width / (f32)height, 0.01f, 100.0f);
  }

  void MainWindow::Setup() {
    m_render_device = GetRenderDevice();

    CreateCommandPoolAndBuffer();
    CreateRenderPass();
    CreateFence();
    CreateGraphicsPipeline();
    CreateVertexAndIndexBuffer();
  }

  void MainWindow::CreateCommandPoolAndBuffer() {
    m_command_pool = m_render_device->CreateGraphicsCommandPool(
      CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer);

    m_render_command_buffer = m_render_device->CreateCommandBuffer(m_command_pool.get());
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

  void MainWindow::CreateFence() {
    m_fence = m_render_device->CreateFence();
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

} // namespace zephyr
