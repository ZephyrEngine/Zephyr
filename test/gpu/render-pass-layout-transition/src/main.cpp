
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/sink/file.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/window/window.hpp>

#include <fmt/format.h>
#include <zephyr/integer.hpp>

#include "shader/mesh.vert.h"
#include "shader/mesh.frag.h"

using namespace zephyr;

class MainWindow final : public Window {
  public:
    MainWindow() {
      SetWindowSize(1600, 900);
      SetWindowTitle("Render Pass Layout Transition");
      Setup();
    }

    void OnFrame() override {
      const auto& render_target = GetSwapChain()->AcquireNextRenderTarget();

      struct Transform {
        Matrix4 projection = Matrix4::PerspectiveVK(45.0f, 1600.0f/900.0f, 0.01f, 100.0f);
        Matrix4 model_view;
      } transform[2];

      transform[0].model_view = Matrix4::Translation(-2, 0, -5) * Matrix4::RotationX(frame * 0.025f);
      transform[1].model_view = Matrix4::Translation(+2, 0, -5) * Matrix4::RotationY(frame * 0.025f);

      render_command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);

      for(int i : {0, 1}) {
        render_pass[i]->SetClearColor(0, 0.02, 0.02, 0.02, 1.0);

        render_command_buffer->BeginRenderPass(render_target.get(), render_pass[i].get());
        render_command_buffer->PushConstants(pipeline->GetLayout(), 0, sizeof(transform[0]), &transform[i]);
        render_command_buffer->BindPipeline(pipeline.get());
        render_command_buffer->BindVertexBuffers({{vbo.get()}});
        render_command_buffer->BindIndexBuffer(ibo.get(), IndexDataType::UInt16);
        render_command_buffer->DrawIndexed(36);
        render_command_buffer->EndRenderPass();
      }
      render_command_buffer->End();

      fence->Reset();
      render_device->GraphicsQueue()->Submit({{render_command_buffer.get()}}, fence.get());
      fence->Wait();

      GetSwapChain()->Present();

      frame++;
    }

  private:
    void Setup() {
      render_device = GetRenderDevice();

      CreateCommandPoolAndBuffer();
      CreateRenderPass();
      CreateFence();
      CreateGraphicsPipeline();
      CreateVertexAndIndexBuffer();
    }

    void CreateCommandPoolAndBuffer() {
      command_pool = render_device->CreateGraphicsCommandPool(
        CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer);

      render_command_buffer = render_device->CreateCommandBuffer(command_pool.get());
    }

    void CreateRenderPass() {
      auto builder = render_device->CreateRenderPassBuilder();

      builder->SetColorAttachmentFormat(0, Texture::Format::B8G8R8A8_SRGB);
      builder->SetColorAttachmentSrcLayout(0, Texture::Layout::Undefined, std::nullopt);
      builder->SetColorAttachmentDstLayout(0, Texture::Layout::PresentSrc, std::nullopt);

      builder->SetDepthAttachmentFormat(Texture::Format::DEPTH_U16);
      builder->SetDepthAttachmentSrcLayout(Texture::Layout::Undefined, std::nullopt);
      builder->SetDepthAttachmentDstLayout(Texture::Layout::DepthStencilAttachment, std::nullopt);

      render_pass[0] = builder->Build();

      builder->SetColorAttachmentSrcLayout(0, Texture::Layout::PresentSrc, {{PipelineStage::ColorAttachmentOutput, Access::ColorAttachmentWrite}});
      builder->SetColorAttachmentLoadOp(0, RenderPass::LoadOp::Load);
      builder->SetDepthAttachmentSrcLayout(Texture::Layout::DepthStencilAttachment, {{PipelineStage::ColorAttachmentOutput, Access::DepthStencilAttachmentWrite}});

      render_pass[1] = builder->Build();
    }

    void CreateFence() {
      fence = render_device->CreateFence(Fence::CreateSignalled::No);
    }

    void CreateGraphicsPipeline() {
      std::shared_ptr<ShaderModule> vert_shader = render_device->CreateShaderModule(mesh_vert, sizeof(mesh_vert));
      std::shared_ptr<ShaderModule> frag_shader = render_device->CreateShaderModule(mesh_frag, sizeof(mesh_frag));

      auto builder = render_device->CreateGraphicsPipelineBuilder();

      builder->SetViewport(0, 0, 1600, 900);
      builder->SetShaderModule(PipelineStage::VertexShader, vert_shader);
      builder->SetShaderModule(PipelineStage::FragmentShader, frag_shader);
      builder->SetRenderPass(render_pass[0]);
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

      auto command_buffer = render_device->CreateCommandBuffer(command_pool.get());

      command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
      command_buffer->CopyBuffer(staging_vbo.get(), vbo.get(), vbo->Size());
      command_buffer->CopyBuffer(staging_ibo.get(), ibo.get(), ibo->Size());
      command_buffer->End();

      render_device->GraphicsQueue()->Submit({{command_buffer.get()}});
      render_device->GraphicsQueue()->WaitIdle();
    }

    std::shared_ptr<RenderDevice> render_device;
    std::shared_ptr<CommandPool> command_pool;
    std::unique_ptr<CommandBuffer> render_command_buffer;
    std::shared_ptr<RenderPass> render_pass[2];
    std::unique_ptr<Fence> fence;
    std::unique_ptr<GraphicsPipeline> pipeline;
    std::shared_ptr<Buffer> vbo;
    std::shared_ptr<Buffer> ibo;

    int frame = 0;
};

int main() {
  zephyr::get_logger().InstallSink(std::make_shared<LoggerConsoleSink>());
  zephyr::get_logger().InstallSink(std::make_shared<LoggerFileSink>("log.txt"));

  ZEPHYR_TRACE("This is a trace log")
  ZEPHYR_DEBUG("This is a debug log")
  ZEPHYR_INFO("This is an info log")
  ZEPHYR_WARN("This is a warning")
  ZEPHYR_ERROR("This is an error")
  ZEPHYR_FATAL("This is a fatal error")

  auto window = std::make_unique<MainWindow>();
  window->Run();

  return 0;
}