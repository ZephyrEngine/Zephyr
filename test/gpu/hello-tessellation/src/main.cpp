
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/window/window.hpp>

#include <fmt/format.h>
#include <zephyr/integer.hpp>

#include "shader/mesh.vert.h"
#include "shader/mesh.tesc.h"
#include "shader/mesh.tese.h"
#include "shader/mesh.frag.h"

using namespace zephyr;

class MainWindow final : public Window {
  public:
    MainWindow() {
      SetWindowSize(1600, 900);
      SetWindowTitle("Hello Tessellation");
      Setup();
    }

    void OnFrame() override {
      const auto& render_target = GetSwapChain()->AcquireNextRenderTarget();

      struct Transform {
        Matrix4 projection = Matrix4::PerspectiveVK(45.0f, 1600.0f/900.0f, 0.01f, 100.0f);
        Matrix4 model_view;
      } transform;

      transform.model_view = Matrix4::Translation(0, 2, -13 + sin(frame * 0.005) * 10.0) * Matrix4::Scale(2.0, 1.0, 2.0);

      render_pass->SetClearColor(0, 0.02, 0.02, 0.02, 1.0);

      render_command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
      render_command_buffer->PushConstants(pipeline->GetLayout(), 0, sizeof(transform), &transform);
      render_command_buffer->BeginRenderPass(render_target.get(), render_pass.get());
      render_command_buffer->BindGraphicsPipeline(pipeline.get());
      render_command_buffer->BindVertexBuffers({{vbo.get()}});
      render_command_buffer->Draw(6);
      render_command_buffer->EndRenderPass();
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
      auto vert_shader = render_device->CreateShaderModule(mesh_vert, sizeof(mesh_vert));
      auto tesc_shader = render_device->CreateShaderModule(mesh_tesc, sizeof(mesh_tesc));
      auto tese_shader = render_device->CreateShaderModule(mesh_tese, sizeof(mesh_tese));
      auto frag_shader = render_device->CreateShaderModule(mesh_frag, sizeof(mesh_frag));

      auto builder = render_device->CreateGraphicsPipelineBuilder();

      builder->SetViewport(0, 0, 1600, 900);
      builder->SetShaderModule(PipelineStage::VertexShader, vert_shader);
      builder->SetShaderModule(PipelineStage::TessellationControlShader, tesc_shader);
      builder->SetShaderModule(PipelineStage::TessellationEvaluationShader, tese_shader);
      builder->SetShaderModule(PipelineStage::FragmentShader, frag_shader);
      builder->SetRenderPass(render_pass);
      builder->SetDepthTestEnable(true);
      builder->SetDepthWriteEnable(true);
      builder->SetDepthCompareOp(CompareOp::LessOrEqual);
      builder->AddVertexInputBinding(0, sizeof(float) * 6);
      builder->AddVertexInputAttribute(0, 0, 0, VertexDataType::Float32, 3, false);
      builder->AddVertexInputAttribute(1, 0, sizeof(float) * 3, VertexDataType::Float32, 3, false);
      builder->SetPolygonMode(PolygonMode::Line);
      builder->SetPrimitiveTopology(PrimitiveTopology::PatchList);
      builder->SetPatchControlPointCount(3);

      pipeline = builder->Build();
    }

    void CreateVertexBuffer() {
      static const float k_vertices[] = {
    //    | POSITION       | COLOR         |
         -1.0, 0.0, -1.0,  1.0, 0.0, 0.0,
         +1.0, 0.0, -1.0,  0.0, 1.0, 0.0,
         -1.0, 0.0, +1.0,  0.0, 0.0, 1.0,

         +1.0, 0.0, -1.0,  0.0, 1.0, 0.0,
         +1.0, 0.0, +1.0,  1.0, 1.0, 1.0,
         -1.0, 0.0, +1.0,  0.0, 0.0, 1.0
      };

      auto staging_vbo = render_device->CreateBuffer(
        Buffer::Usage::CopySrc, Buffer::Flags::HostVisible, sizeof(k_vertices));

      vbo = render_device->CreateBuffer(
        Buffer::Usage::VertexBuffer | Buffer::Usage::CopyDst, Buffer::Flags::None, sizeof(k_vertices));

      // @todo: fix the API so that we either have to Map() the buffer, or don't have to unmap it.
      staging_vbo->Update<u8>((u8 const*)k_vertices, sizeof(k_vertices));
      staging_vbo->Unmap();

      auto command_buffer = render_device->CreateCommandBuffer(command_pool.get());

      command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
      command_buffer->CopyBuffer(staging_vbo.get(), vbo.get(), vbo->Size());
      command_buffer->End();

      render_device->GraphicsQueue()->Submit({{command_buffer.get()}});
      render_device->GraphicsQueue()->WaitIdle();
    }

    std::shared_ptr<RenderDevice> render_device;
    std::shared_ptr<CommandPool> command_pool;
    std::unique_ptr<CommandBuffer> render_command_buffer;
    std::shared_ptr<RenderPass> render_pass;
    std::unique_ptr<Fence> fence;
    std::unique_ptr<GraphicsPipeline> pipeline;
    std::shared_ptr<Buffer> vbo;

    int frame = 0;
};

int main() {
  zephyr::get_logger().InstallSink(std::make_shared<LoggerConsoleSink>());

  auto window = std::make_unique<MainWindow>();
  window->Run();

  return 0;
}