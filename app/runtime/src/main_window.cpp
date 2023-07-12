
#include "main_window.hpp"

namespace zephyr {

  MainWindow::MainWindow() {
    SetWindowSize(1600, 900);
    SetWindowTitle("C(omp)ute rasterization");
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

    Texture* texture = render_target->GetColorAttachments()[0]->GetTexture();

    bind_group->Bind(0u, texture, Texture::Layout::General, BindingType::StorageImage);
    bind_group->Bind(1u, m_buffer_cache->GetDeviceBuffer(m_vertex_ssbo.get()), BindingType::StorageBuffer);

    command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
    {
      // @todo: round the thread group sizes up
      const size_t k_tile_size = 16u;
      const size_t thread_group_size_x = m_width / k_tile_size;
      const size_t thread_group_size_y = m_height / k_tile_size;
      command_buffer->Barrier(texture, PipelineStage::TopOfPipe, PipelineStage::ComputeShader, Access::None, Access::ShaderWrite, Texture::Layout::Undefined, Texture::Layout::General, 0u, 1u);
      command_buffer->BindPipeline(m_compute_pipeline.get());
      command_buffer->BindBindGroup(PipelineBindPoint::Compute, m_compute_pipeline->GetLayout(), 0u, bind_group.get());
      command_buffer->DispatchCompute(thread_group_size_x, thread_group_size_y);
      command_buffer->Barrier(texture, PipelineStage::ComputeShader, PipelineStage::BottomOfPipe, Access::ShaderWrite, Access::None, Texture::Layout::General, Texture::Layout::PresentSrc, 0u, 1u);
    }
    command_buffer->End();

    m_resource_uploader->FinalizeFrame();

    // @todo: remove TmpWaitForImageFullyRead() completely and use a semaphore instead.
    GetSwapChain()->TmpWaitForImageFullyRead();
    fence->Reset();
    m_render_device->GraphicsQueue()->Submit({{m_resource_uploader->GetCurrentCommandBuffer(), command_buffer.get()}}, fence.get());

    GetSwapChain()->Present();

    m_frame++;

    UpdateFramesPerSecondCounter();
  }

  void MainWindow::OnResize(int width, int height) {
    m_width = width;
    m_height = height;
  }

  void MainWindow::Setup() {
    m_render_device = GetRenderDevice();

    m_frames_in_flight = GetSwapChain()->GetNumberOfSwapChainImages();

    CreateCommandPoolAndBuffers();
    CreateResourceUploader();
    CreateBufferCache();
    CreateTextureCache();
    CreateFences();
    CreateBindGroups();
    CreateComputePipeline();
    CreateVertexSSBO();
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

  void MainWindow::CreateTextureCache() {
    m_texture_cache = std::make_shared<TextureCache>(m_render_device, m_resource_uploader);
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
        .type = BindingType::StorageImage,
        .stages = ShaderStage::Compute
      },
      {
        .binding = 1u,
        .type = BindingType::StorageBuffer
      }
    }});

    for(uint i = 0; i < m_frames_in_flight; i++) {
      m_bind_groups.push_back(m_render_device->CreateBindGroup(m_bind_group_layout));
    }
  }

  void MainWindow::CreateComputePipeline() {
    const std::shared_ptr<PipelineLayout> pipeline_layout = m_render_device->CreatePipelineLayout({{m_bind_group_layout.get()}});

    static const std::shared_ptr<ShaderModule> shader_module = m_render_device->CreateShaderModule(rasterize_comp, sizeof(rasterize_comp));

    // @todo: CreateComputePipeline() probably should take a std::shared_ptr
    m_compute_pipeline = m_render_device->CreateComputePipeline(shader_module.get(), pipeline_layout);
  }

  void MainWindow::CreateVertexSSBO() {
    struct Vertex {
      f32 position[4];
      f32 color[4];
    };

    Vertex vertices[] {
      {.position = { 0.00f, -1.0f,  0.0f,  1.0f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}},
      {.position = {-0.75f,  1.0f,  0.0f,  1.0f}, .color = {0.0f, 1.0f, 0.0f, 1.0f}},
      {.position = { 0.75f,  0.5f,  0.0f,  1.0f}, .color = {0.0f, 0.0f, 1.0f, 1.0f}},

      {.position = {-1.00f, -1.0f,  0.0f,  1.0f}, .color = {1.0f, 1.0f, 0.0f, 1.0f}},
      {.position = { 0.00f, -1.0f,  0.0f,  1.0f}, .color = {0.0f, 1.0f, 1.0f, 1.0f}},
      {.position = {-1.00f,  1.0f,  0.0f,  1.0f}, .color = {1.0f, 0.0f, 1.0f, 1.0f}}
    };

    m_vertex_ssbo = std::make_unique<BufferResource>(Buffer::Usage::StorageBuffer, std::span{(u8*)vertices, sizeof(vertices)});
  }

  void MainWindow::UpdateFramesPerSecondCounter() {
    const auto time_point_now = std::chrono::steady_clock::now();

    const auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      time_point_now - m_time_point_last_update).count();

    m_fps_counter++;

    if(time_elapsed >= 1000) {
      const f32 fps = (f32)m_fps_counter * 1000.0f / (f32)time_elapsed;
      SetWindowTitle(fmt::format("C(omp)ute rasterization ({} fps)", fps));
      m_fps_counter = 0;
      m_time_point_last_update = std::chrono::steady_clock::now();
    }
  }

} // namespace zephyr
