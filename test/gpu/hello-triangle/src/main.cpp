
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/window/window.hpp>

#include <fmt/format.h>
#include <zephyr/integer.hpp>

#include "shader/triangle.vert.h"
#include "shader/triangle.frag.h"

using namespace zephyr;

class MainWindow final : public Window {
  public:
    MainWindow() {
      SetWindowSize(1600, 900);
      SetWindowTitle("Hello Triangle");
      Setup();
    }

    void OnFrame() override {
      const auto& render_target = GetSwapChain()->AcquireNextRenderTarget();

      render_pass->SetClearColor(0, 1.0, 0.0, 1.0, 1.0);

      render_command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
      render_command_buffer->BeginRenderPass(render_target.get(), render_pass.get());
      render_command_buffer->BindGraphicsPipeline(pipeline.get());
      render_command_buffer->BindVertexBuffers({{vbo.get()}});
      render_command_buffer->Draw(3);
      render_command_buffer->EndRenderPass();
      render_command_buffer->End();

      fence->Reset();
      render_device->GraphicsQueue()->Submit({{render_command_buffer.get()}}, fence.get());
      fence->Wait();

      GetSwapChain()->Present();
    }

  private:
    void Setup() {
      render_device = GetRenderDevice();

      CreateCommandPoolAndBuffer();
      CreateRenderPass();
      CreateFence();
      CreateGraphicsPipeline();
      CreateVertexBuffer();
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

      render_pass = builder->Build();
    }

    void CreateFence() {
      fence = render_device->CreateFence(Fence::CreateSignalled::No);
    }

    void CreateGraphicsPipeline() {
      std::shared_ptr<ShaderModule> vert_shader = render_device->CreateShaderModule(triangle_vert, sizeof(triangle_vert));
      std::shared_ptr<ShaderModule> frag_shader = render_device->CreateShaderModule(triangle_frag, sizeof(triangle_frag));

      auto builder = render_device->CreateGraphicsPipelineBuilder();

      builder->SetViewport(0, 0, 1600, 900);
      builder->SetShaderModule(PipelineStage::VertexShader, vert_shader);
      builder->SetShaderModule(PipelineStage::FragmentShader, frag_shader);
      builder->SetRenderPass(render_pass);
      builder->AddVertexInputBinding(0, sizeof(float) * 6);
      builder->AddVertexInputAttribute(0, 0, 0, VertexDataType::Float32, 3, false);
      builder->AddVertexInputAttribute(1, 0, sizeof(float) * 3, VertexDataType::Float32, 3, false);

      pipeline = builder->Build();
    }

    void CreateVertexBuffer() {
      static const float k_vertices[] = {
        // POSITION    | COLOR
        -1.0,  1.0, 0.0,  1.0, 0.0, 0.0,
         1.0,  1.0, 0.0,  0.0, 1.0, 0.0,
         0.0, -1.0, 0.0,  0.0, 0.0, 1.0
      };

      auto staging_buffer = render_device->CreateBuffer(
        Buffer::Usage::CopySrc, Buffer::Flags::HostVisible, sizeof(k_vertices));

      vbo = render_device->CreateBuffer(
        Buffer::Usage::VertexBuffer | Buffer::Usage::CopyDst, Buffer::Flags::None, sizeof(k_vertices));

      staging_buffer->Update<u8>((u8 const*)k_vertices, sizeof(k_vertices));
      staging_buffer->Unmap();

      auto command_buffer = render_device->CreateCommandBuffer(command_pool.get());

      command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
      command_buffer->CopyBuffer(staging_buffer.get(), vbo.get(), vbo->Size());
      command_buffer->End();

      render_device->GraphicsQueue()->Submit({{command_buffer.get()}});
      render_device->GraphicsQueue()->WaitIdle();
    }

    std::shared_ptr<RenderDevice> render_device;
    std::shared_ptr<CommandPool> command_pool;
    std::shared_ptr<CommandBuffer> render_command_buffer;
    std::shared_ptr<RenderPass> render_pass;
    std::unique_ptr<Fence> fence;
    std::unique_ptr<GraphicsPipeline> pipeline;
    std::shared_ptr<Buffer> vbo;
};

int main() {
  zephyr::get_logger().InstallSink(std::make_shared<LoggerConsoleSink>());

  auto window = std::make_unique<MainWindow>();
  window->Run();

  return 0;
}