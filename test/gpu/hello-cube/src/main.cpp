
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/window/window.hpp>

#include <chrono>
#include <fmt/format.h>
#include <zephyr/integer.hpp>
#include <zephyr/float.hpp>
#include <zephyr/panic.hpp>

#include <thread>

#include "shader/mesh.vert.h"
#include "shader/mesh.frag.h"

using namespace zephyr;

/// BEGIN SETTINGS

static constexpr int g_thread_count = 4;

enum class Mode {
  ST,
  MT, // Multi-threaded, recording directly into Vulkan native command buffers
  MT_irc // Multi-threaded, using intermediate render command lists
} g_mode = Mode::MT_irc;

/// END SETTINGS

enum RenderCommandID : u8 {
  BeginRenderPass,
  EndRenderPass,
  BindPipeline,
  BindVertexBuffer,
  BindIndexBuffer,
  PushConstants,
  DrawIndexed
};

struct BeginRenderPassCmd {
  RenderTarget* render_target;
  RenderPass* render_pass;
};

struct BindPipelineCmd {
  GraphicsPipeline* pipeline;
};

struct BindVertexBufferCmd {
  u32 index;
  Buffer* buffer;
};

struct BindIndexBufferCmd {
  IndexDataType data_type;
  Buffer* buffer;
};

struct PushConstantsCmd {
  PipelineLayout* layout;
  u32 offset;
  u32 size;
//  u8* data;
  int push_index;
};

struct DrawIndexedCmd {
  u32 index_count;
};

struct RenderCommand {
  RenderCommandID cmd;

  union {
    BeginRenderPassCmd begin_render_pass;
    BindPipelineCmd bind_pipeline;
    BindVertexBufferCmd bind_vertex_buffer;
    BindIndexBufferCmd bind_index_buffer;
    PushConstantsCmd push_constants;
    DrawIndexedCmd draw_indexed;
  } param;
};

class RenderCommandList {
  public:
    RenderCommandList() {
      m_push_bump.reserve(131072); // overkill
      m_commands.reserve(131072);
    }

    void Reset() {
      m_push_bump.clear();
      m_commands.clear();
    }

    void BeginRenderPass(RenderTarget* render_target, RenderPass* render_pass) {
      m_commands.emplace_back();

      RenderCommand& command = m_commands.back();
      command.cmd = RenderCommandID::BeginRenderPass;
      command.param.begin_render_pass.render_target = render_target;
      command.param.begin_render_pass.render_pass = render_pass;
    }

    void EndRenderPass() {
      m_commands.emplace_back();
      m_commands.back().cmd = RenderCommandID::EndRenderPass;
    }

    void BindPipeline(GraphicsPipeline* pipeline) {
      m_commands.emplace_back();

      RenderCommand& command = m_commands.back();
      command.cmd = RenderCommandID::BindPipeline;
      command.param.bind_pipeline.pipeline = pipeline;
    }

    void BindVertexBuffer(u32 index, Buffer* buffer) {
      m_commands.emplace_back();

      RenderCommand& command = m_commands.back();
      command.cmd = RenderCommandID::BindVertexBuffer;
      command.param.bind_vertex_buffer.index = index;
      command.param.bind_vertex_buffer.buffer = buffer;
    }

    void BindIndexBuffer(IndexDataType data_type, Buffer* buffer) {
      m_commands.emplace_back();

      RenderCommand& command = m_commands.back();
      command.cmd = RenderCommandID::BindIndexBuffer;
      command.param.bind_index_buffer.data_type = data_type;
      command.param.bind_index_buffer.buffer = buffer;
    }

    void PushConstants(PipelineLayout* layout, u32 offset, u32 size, const void* data) {
      m_push_bump.push_back({});
      std::memcpy(m_push_bump.back().data(), data, size);

      m_commands.emplace_back();

      RenderCommand& command = m_commands.back();
      command.cmd = RenderCommandID::PushConstants;
      command.param.push_constants.layout = layout;
      command.param.push_constants.offset = offset;
      command.param.push_constants.size = size;
      //command.param.push_constants.data = data; // @todo: I guess taking ownership would be appropriate
//      command.param.push_constants.data = m_push_bump.back().data();
      command.param.push_constants.push_index = m_push_bump.size() - 1;
    }

    void DrawIndexed(u32 index_count) {
      m_commands.emplace_back();

      RenderCommand& command = m_commands.back();
      command.cmd = RenderCommandID::DrawIndexed;
      command.param.draw_indexed.index_count = index_count;
    }

    void Translate(CommandBuffer* cmd_buffer) const {
      cmd_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);

      for(const RenderCommand& command : m_commands) {
        switch(command.cmd) {
          case RenderCommandID::BeginRenderPass: {
            cmd_buffer->BeginRenderPass(
              command.param.begin_render_pass.render_target,
              command.param.begin_render_pass.render_pass
            );
            break;
          }
          case RenderCommandID::EndRenderPass: {
            cmd_buffer->EndRenderPass();
            break;
          }
          case RenderCommandID::BindPipeline: {
            cmd_buffer->BindPipeline(command.param.bind_pipeline.pipeline);
            break;
          }
          case RenderCommandID::BindVertexBuffer: {
            cmd_buffer->BindVertexBuffers({{command.param.bind_vertex_buffer.buffer}}, command.param.bind_vertex_buffer.index);
            break;
          }
          case RenderCommandID::BindIndexBuffer: {
            cmd_buffer->BindIndexBuffer(command.param.bind_index_buffer.buffer, command.param.bind_index_buffer.data_type);
            break;
          }
          case RenderCommandID::PushConstants: {
            cmd_buffer->PushConstants(
              command.param.push_constants.layout,
              command.param.push_constants.offset,
              command.param.push_constants.size,
              m_push_bump[command.param.push_constants.push_index].data()
            );
            break;
          }
          case RenderCommandID::DrawIndexed: {
            cmd_buffer->DrawIndexed(command.param.draw_indexed.index_count);
            break;
          }
          default: {
            ZEPHYR_PANIC("Unimplemented render command: {}", (int)command.cmd);
          }
        }
      }

      cmd_buffer->End();
    }

  private:
    // @todo: this just a really *bad* stub. Replace my intrusive linked list or something later.
    std::vector<RenderCommand> m_commands;

    std::vector<std::array<u8, 128>> m_push_bump;
};

class MainWindow final : public Window {
  public:
    MainWindow() {
      SetWindowSize(1600, 900);
      SetWindowTitle("Hello Cube");
      Setup();
    }

    void OnFrame() override {
      const int frame_index = frame % m_frames_in_flight;

      Fence* fence = m_fences[frame_index].get();

      fence->Wait();

      const auto& render_target = GetSwapChain()->AcquireNextRenderTarget();

      struct Transform {
        Matrix4 projection = Matrix4::PerspectiveVK(45.0f, 1600.0f/900.0f, 0.01f, 100.0f);
        Matrix4 model_view;
      };

      render_pass_clear->SetClearColor(0, 0.02, 0.02, 0.02, 1.0);

      // 40k cubes
      const int x_cubes = 200;
      const int y_cubes = 20;
      const int z_cubes = 10;
      const f32 spread = 0.3f;

      if((x_cubes) % g_thread_count != 0) {
        ZEPHYR_PANIC("x_cubes not divisible by thread count !");
      }

      switch(g_mode) {
        case Mode::ST: {
          // Single-threaded
          CommandBuffer* render_command_buffer = m_render_command_buffers[frame_index].get();

          Transform transform;
          transform.model_view = Matrix4::Translation(0, 0, -5);

          render_command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
          render_command_buffer->BeginRenderPass(render_target.get(), render_pass_clear.get());

          for(int x = 0; x < x_cubes; x++) {
            for(int y = 0; y < y_cubes; y++) {
              for(int z = 0; z < z_cubes; z++) {
                transform.model_view = Matrix4::Translation(
                  ((f32)x - (f32)x_cubes * 0.5f) * spread,
                  ((f32)y - (f32)y_cubes * 0.5f) * spread,
                  -5.0f - (f32)z * spread
                ) * Matrix4::Scale(0.1f, 0.1f, 0.1f);

                render_command_buffer->BindPipeline(pipeline.get());
                render_command_buffer->BindVertexBuffers({{vbo.get()}});
                render_command_buffer->BindIndexBuffer(ibo.get(), IndexDataType::UInt16);
                render_command_buffer->PushConstants(pipeline->GetLayout(), 0, sizeof(transform), &transform);
                render_command_buffer->DrawIndexed(36);
              }
            }
          }

          render_command_buffer->EndRenderPass();
          render_command_buffer->End();

          // @todo: remove TmpWaitForImageFullyRead() completely and use a semaphore instead.
          GetSwapChain()->TmpWaitForImageFullyRead();
          fence->Reset();
          render_device->GraphicsQueue()->Submit({{render_command_buffer}}, fence);
          break;
        }
        case Mode::MT: {
          // Multi-threaded
          const int split_cubes = x_cubes / g_thread_count;

          std::vector<std::thread> threads;

          std::vector<CommandBuffer*> cmd_bufs;

          for(int i = 0; i < g_thread_count; i++) {
            CommandBuffer* render_command_buffer = m_render_command_buffers[frame_index * g_thread_count + i].get();

            cmd_bufs.push_back(render_command_buffer);

            threads.emplace_back([=]{
              Transform transform;
              transform.model_view = Matrix4::Translation(0, 0, -5);

              render_command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
              if(i == 0) {
                render_command_buffer->BeginRenderPass(render_target.get(), render_pass_clear.get());
              } else {
                render_command_buffer->BeginRenderPass(render_target.get(), render_pass_load.get());
              }

              const int x0 = split_cubes * i;
              const int x1 = x0 + split_cubes;

              for(int x = x0; x < x1; x++) {
                for(int y = 0; y < y_cubes; y++) {
                  for(int z = 0; z < z_cubes; z++) {
                    transform.model_view = Matrix4::Translation(
                      ((f32)x - (f32)x_cubes * 0.5f) * spread,
                      ((f32)y - (f32)y_cubes * 0.5f) * spread,
                      -5.0f - (f32)z * spread
                    ) * Matrix4::Scale(0.1f, 0.1f, 0.1f);

                    render_command_buffer->BindPipeline(pipeline.get());
                    render_command_buffer->BindVertexBuffers({{vbo.get()}});
                    render_command_buffer->BindIndexBuffer(ibo.get(), IndexDataType::UInt16);
                    render_command_buffer->PushConstants(pipeline->GetLayout(), 0, sizeof(transform), &transform);
                    render_command_buffer->DrawIndexed(36);
                  }
                }
              }

              render_command_buffer->EndRenderPass();
              render_command_buffer->End();
            });
          }

          for(auto& thread : threads) thread.join();

          // @todo: remove TmpWaitForImageFullyRead() completely and use a semaphore instead.
          GetSwapChain()->TmpWaitForImageFullyRead();
          fence->Reset();
          // @todo: do not hardcode number of threads.
          render_device->GraphicsQueue()->Submit({cmd_bufs}, fence);
          break;
        }
        case Mode::MT_irc: {
          // Multi-threaded, intermediate render command lists
          const int split_cubes = x_cubes / g_thread_count;

          std::vector<std::thread> threads;

          std::vector<CommandBuffer*> cmd_bufs;

          for(int i = 0; i < g_thread_count; i++) {
            CommandBuffer* render_command_buffer = m_render_command_buffers[frame_index * g_thread_count + i].get();

            cmd_bufs.push_back(render_command_buffer);

            RenderCommandList* render_command_list = &render_command_lists[i];

            threads.emplace_back([=]{
              Transform transform;
              transform.model_view = Matrix4::Translation(0, 0, -5);

              render_command_list->Reset();
              if(i == 0) {
                render_command_list->BeginRenderPass(render_target.get(), render_pass_clear.get());
              } else {
                render_command_list->BeginRenderPass(render_target.get(), render_pass_load.get());
              }

              const int x0 = split_cubes * i;
              const int x1 = x0 + split_cubes;

              for(int x = x0; x < x1; x++) {
                for(int y = 0; y < y_cubes; y++) {
                  for(int z = 0; z < z_cubes; z++) {
                    transform.model_view = Matrix4::Translation(
                      ((f32)x - (f32)x_cubes * 0.5f) * spread,
                      ((f32)y - (f32)y_cubes * 0.5f) * spread,
                      -5.0f - (f32)z * spread
                    ) * Matrix4::Scale(0.1f, 0.1f, 0.1f);

                    render_command_list->BindPipeline(pipeline.get());
                    render_command_list->BindVertexBuffer(0, vbo.get());
                    render_command_list->BindIndexBuffer(IndexDataType::UInt16, ibo.get());
                    render_command_list->PushConstants(pipeline->GetLayout(), 0, sizeof(transform), &transform);
                    render_command_list->DrawIndexed(36);
                  }
                }
              }

              render_command_list->EndRenderPass();

              render_command_list->Translate(render_command_buffer);
            });
          }

          for(auto& thread : threads) thread.join();

          // @todo: remove TmpWaitForImageFullyRead() completely and use a semaphore instead.
          GetSwapChain()->TmpWaitForImageFullyRead();
          fence->Reset();
          // @todo: do not hardcode number of threads.
          render_device->GraphicsQueue()->Submit({cmd_bufs}, fence);
          break;
        }
      }

      GetSwapChain()->Present();

      frame++;

      UpdateFramesPerSecondCounter();
    }

  private:
    void Setup() {
      render_device = GetRenderDevice();

      m_frames_in_flight = GetSwapChain()->GetNumberOfSwapChainImages();
      ZEPHYR_INFO("Renderer configured to have {} frame(s) in flight", m_frames_in_flight);

      CreateCommandPoolAndBuffers();
      CreateRenderPass();
      CreateFences();
      CreateGraphicsPipeline();
      CreateVertexAndIndexBuffer();
    }

    void CreateCommandPoolAndBuffers() {
      for(int i = 0; i < g_thread_count; i++) {
        m_render_command_pools.push_back(render_device->CreateGraphicsCommandPool(
          CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer));
      }

      for(uint i = 0; i < m_frames_in_flight; i++) {
        for(int j = 0; j < g_thread_count; j++) {
          m_render_command_buffers.push_back(render_device->CreateCommandBuffer(m_render_command_pools[j]));
        }
      }
    }

    void CreateRenderPass() {
      auto builder = render_device->CreateRenderPassBuilder();

      builder->SetColorAttachmentFormat(0, Texture::Format::B8G8R8A8_SRGB);
      builder->SetColorAttachmentSrcLayout(0, Texture::Layout::Undefined, std::nullopt);
      builder->SetColorAttachmentDstLayout(0, Texture::Layout::PresentSrc, std::nullopt);
      builder->SetDepthAttachmentFormat(Texture::Format::DEPTH_U16);
      builder->SetDepthAttachmentSrcLayout(Texture::Layout::Undefined, std::nullopt);
      builder->SetDepthAttachmentDstLayout(Texture::Layout::DepthStencilAttachment, std::nullopt);
      render_pass_clear = builder->Build();

      builder->SetColorAttachmentLoadOp(0, RenderPass::LoadOp::Load);
      builder->SetColorAttachmentSrcLayout(0, Texture::Layout::PresentSrc, RenderPassBuilder::Transition{PipelineStage::ColorAttachmentOutput, Access::ColorAttachmentWrite});
      builder->SetColorAttachmentDstLayout(0, Texture::Layout::PresentSrc, RenderPassBuilder::Transition{PipelineStage::ColorAttachmentOutput, Access::ColorAttachmentWrite});
      builder->SetDepthAttachmentLoadOp(RenderPass::LoadOp::Load);
      builder->SetDepthAttachmentSrcLayout(Texture::Layout::DepthStencilAttachment, std::nullopt);
      render_pass_load = builder->Build();
    }

    void CreateFences() {
      for(uint i = 0; i < m_frames_in_flight; i++) {
        m_fences.push_back(render_device->CreateFence(Fence::CreateSignalled::Yes));
      }
    }

    void CreateGraphicsPipeline() {
      std::shared_ptr<ShaderModule> vert_shader = render_device->CreateShaderModule(mesh_vert, sizeof(mesh_vert));
      std::shared_ptr<ShaderModule> frag_shader = render_device->CreateShaderModule(mesh_frag, sizeof(mesh_frag));

      auto builder = render_device->CreateGraphicsPipelineBuilder();

      builder->SetViewport(0, 0, 1600, 900);
      builder->SetShaderModule(ShaderStage::Vertex, vert_shader);
      builder->SetShaderModule(ShaderStage::Fragment, frag_shader);
      builder->SetRenderPass(render_pass_clear);
      builder->SetDepthTestEnable(true);
      builder->SetDepthWriteEnable(true);
      builder->SetDepthCompareOp(CompareOp::LessOrEqual);
      builder->AddVertexInputBinding(0, sizeof(float) * 6);
      builder->AddVertexInputAttribute(0, 0, 0, VertexDataType::Float32, 3, false);
      builder->AddVertexInputAttribute(1, 0, sizeof(float) * 3, VertexDataType::Float32, 3, false);

      pipeline = builder->Build();
    }

    void CreateVertexAndIndexBuffer() {
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

      auto staging_vbo = render_device->CreateBuffer(
        Buffer::Usage::CopySrc, Buffer::Flags::HostVisible, sizeof(k_vertices));

      auto staging_ibo = render_device->CreateBuffer(
        Buffer::Usage::CopySrc, Buffer::Flags::HostVisible, sizeof(k_indices));

      vbo = render_device->CreateBuffer(
        Buffer::Usage::VertexBuffer | Buffer::Usage::CopyDst, Buffer::Flags::None, sizeof(k_vertices));

      ibo = render_device->CreateBuffer(
        Buffer::Usage::IndexBuffer | Buffer::Usage::CopyDst, Buffer::Flags::None, sizeof(k_indices));

      // @todo: fix the API so that we either have to Map() the buffer, or don't have to unmap it.
      staging_vbo->Update<u8>((u8 const*)k_vertices, sizeof(k_vertices));
      staging_vbo->Unmap();

      staging_ibo->Update<u8>((u8 const*)k_indices, sizeof(k_indices));
      staging_ibo->Unmap();

      auto command_buffer = render_device->CreateCommandBuffer(m_render_command_pools[0]);

      command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
      command_buffer->CopyBuffer(staging_vbo.get(), vbo.get(), vbo->Size());
      command_buffer->CopyBuffer(staging_ibo.get(), ibo.get(), ibo->Size());
      command_buffer->End();

      render_device->GraphicsQueue()->Submit({{command_buffer.get()}});
      render_device->GraphicsQueue()->WaitIdle();
    }

    void UpdateFramesPerSecondCounter() {
      const auto time_point_now = std::chrono::steady_clock::now();

      const auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        time_point_now - m_time_point_last_update).count();

      m_fps_counter++;

      if(time_elapsed >= 1000) {
        const f32 fps = (f32)m_fps_counter * 1000.0f / (f32)time_elapsed;
        SetWindowTitle(fmt::format("Hello Cube ({} fps)", fps));
        m_fps_counter = 0;
        m_time_point_last_update = std::chrono::steady_clock::now();
      }
    }

    std::shared_ptr<RenderDevice> render_device;
    std::vector<std::shared_ptr<CommandPool>> m_render_command_pools;
    std::vector<std::unique_ptr<CommandBuffer>> m_render_command_buffers;
    std::shared_ptr<RenderPass> render_pass_clear;
    std::shared_ptr<RenderPass> render_pass_load;
    std::vector<std::unique_ptr<Fence>> m_fences;
    std::unique_ptr<GraphicsPipeline> pipeline;
    std::shared_ptr<Buffer> vbo;
    std::shared_ptr<Buffer> ibo;

    int m_fps_counter{};
    std::chrono::steady_clock::time_point m_time_point_last_update;

    int frame = 0;

    uint m_frames_in_flight{};

    RenderCommandList render_command_lists[g_thread_count];
};

int main() {
  zephyr::get_logger().InstallSink(std::make_shared<LoggerConsoleSink>());

  auto window = std::make_unique<MainWindow>();
  window->Run();

  return 0;
}