
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

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

    static f32 rotation = 0;

    m_ubo->Data<Matrix4>()[0] = Matrix4::Translation(0.0f, 0.0f, -3.0f) * Matrix4::RotationY(rotation);
    m_ubo->Data<Matrix4>()[1] = Matrix4::PerspectiveVK(45.0f, (f32)m_width / (f32)m_height, 0.01f, 100.0f);
    m_ubo->MarkAsDirty();
    rotation += 0.01;

    bind_group->Bind(0u, texture, Texture::Layout::General, BindingType::StorageImage);
    bind_group->Bind(1u, m_buffer_cache->GetDeviceBuffer(m_vertex_ssbo.get()), BindingType::StorageBuffer);
    bind_group->Bind(2u, m_buffer_cache->GetDeviceBuffer(m_index_ssbo.get()), BindingType::StorageBuffer);
    bind_group->Bind(3u, m_buffer_cache->GetDeviceBuffer(m_ubo.get()), BindingType::UniformBuffer);

    command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
    {
      // @todo: round the thread group sizes up
      const size_t k_tile_size = 16u;
      const size_t k_pixels_per_tile = 4;
      const size_t thread_group_size_x = m_width / k_tile_size /k_pixels_per_tile;
      const size_t thread_group_size_y = m_height / k_tile_size /k_pixels_per_tile;

      u32 triangle_count = m_index_ssbo->Size() / sizeof(u32) / 3;

      command_buffer->Barrier(texture, PipelineStage::TopOfPipe, PipelineStage::ComputeShader, Access::None, Access::ShaderWrite, Texture::Layout::Undefined, Texture::Layout::General, 0u, 1u);
      command_buffer->BindPipeline(m_compute_pipeline.get());
      command_buffer->BindBindGroup(PipelineBindPoint::Compute, m_compute_pipeline->GetLayout(), 0u, bind_group.get());
      command_buffer->PushConstants(m_compute_pipeline->GetLayout(), 0u, sizeof(u32), &triangle_count);
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
    CreateVertexAndIndexSSBOs();
    CreateUBO();
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
      },
      {
        .binding = 2u,
        .type = BindingType::StorageBuffer
      },
      {
        .binding = 3u,
        .type = BindingType::UniformBuffer
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

  void MainWindow::CreateVertexAndIndexSSBOs() {
    struct Vertex {
      f32 position[4];
      f32 color[4];
    };

    /**
     *   4-------5
     *  /|      /|
     * 0-------1 |
     * | 6-----|-7
     * |/      |/
     * 2-------3
     */

    /*
    Vertex vertices[] {
      // front face
      {.position = {-1.0, -1.0,  1.0, 1.0f}, .color = {1.0, 0.0, 0.0, 1.0}},
      {.position = { 1.0, -1.0,  1.0, 1.0f}, .color = {0.0, 1.0, 0.0, 1.0}},
      {.position = {-1.0,  1.0,  1.0, 1.0f}, .color = {1.0, 0.0, 1.0, 1.0}},
      {.position = { 1.0,  1.0,  1.0, 1.0f}, .color = {0.5, 0.5, 0.5, 1.0}},

      // back face
      {.position = {-1.0, -1.0, -1.0, 1.0f}, .color = {1.0, 1.0, 0.0, 1.0}},
      {.position = { 1.0, -1.0, -1.0, 1.0f}, .color = {1.0, 0.0, 1.0, 1.0}},
      {.position = {-1.0,  1.0, -1.0, 1.0f}, .color = {0.0, 1.0, 1.0, 1.0}},
      {.position = { 1.0,  1.0, -1.0, 1.0f}, .color = {1.0, 1.0, 1.0, 1.0}}
    };

    u32 indices[] {
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

    m_vertex_ssbo = std::make_unique<BufferResource>(Buffer::Usage::StorageBuffer, std::span{(u8*)vertices, sizeof(vertices)});
    m_index_ssbo = std::make_unique<BufferResource>(Buffer::Usage::StorageBuffer, std::span{(u8*)indices, sizeof(indices)});
    */

    const std::string input_file = "sponza.obj";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, input_file.c_str());

    if(!warn.empty()) {
      ZEPHYR_WARN("{}", warn);
    }
    if(!err.empty()) {
      ZEPHYR_ERROR("{}", err);
    }
    if(!ret) {
      ZEPHYR_PANIC("Unknown error during OBJ loading");
    }

    if(shapes.empty()) {
      ZEPHYR_PANIC("No shapes found in the OBJ file");
    }

    std::vector<Vertex> vertices2;
    std::vector<u32> indices2;

    tinyobj::shape_t& shape = shapes[0];

    // Loop over faces (polygon)
    size_t index_offset = 0;
    for(size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
      size_t fv = (size_t)shape.mesh.num_face_vertices[f];

      // Loop over vertices in the face
      for(size_t v = 0; v < fv; v++) {
        tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

        const f32 vx = attrib.vertices[3 * (size_t)idx.vertex_index + 0];
        const f32 vy = -attrib.vertices[3 * (size_t)idx.vertex_index + 1];
        const f32 vz = attrib.vertices[3 * (size_t)idx.vertex_index + 2];

        vertices2.push_back(Vertex{.position = {vx, vy, vz, 1.0f}, .color={std::rand()/(f32)RAND_MAX, std::rand()/(f32)RAND_MAX, std::rand()/(f32)RAND_MAX, 1.0}});
        indices2.push_back(indices2.size());
      }

      if(fv != 3) {
        ZEPHYR_PANIC("Polygon must be a triangle");
      }

      index_offset += fv;
    }



    m_vertex_ssbo = std::make_unique<BufferResource>(Buffer::Usage::StorageBuffer, std::span{(u8*)vertices2.data(), vertices2.size() * sizeof(Vertex)});
    m_index_ssbo = std::make_unique<BufferResource>(Buffer::Usage::StorageBuffer, std::span{(u8*)indices2.data(), indices2.size() * sizeof(u32)});
  }

  void MainWindow::CreateUBO() {
    m_ubo = std::make_unique<BufferResource>(Buffer::Usage::UniformBuffer, sizeof(Matrix4) * 2);
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
