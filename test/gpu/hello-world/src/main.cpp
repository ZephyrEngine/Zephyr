
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/window/window.hpp>

#include <fmt/format.h>

using namespace zephyr;

class MainWindow final : public Window {
  public:
    MainWindow() {
      SetWindowSize(1600, 900);
      SetWindowTitle("Hello World");
      Setup();
    }

    void OnFrame() override {
      const auto& render_target = GetSwapChain()->AcquireNextRenderTarget();

      render_pass->SetClearColor(0, 1.0, 0.0, 1.0, 1.0);

      command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
      command_buffer->BeginRenderPass(render_target.get(), render_pass.get());
      command_buffer->EndRenderPass();
      command_buffer->End();

      fence->Reset();
      render_device->GraphicsQueue()->Submit({{command_buffer.get()}}, fence.get());
      fence->Wait();

      GetSwapChain()->Present();
    }

  private:
    void Setup() {
      render_device = GetRenderDevice();

      command_pool = render_device->CreateGraphicsCommandPool(
        CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer);

      command_buffer = render_device->CreateCommandBuffer(command_pool.get());

      auto builder = render_device->CreateRenderPassBuilder();

      builder->SetColorAttachmentFormat(0, Texture::Format::B8G8R8A8_SRGB);
      builder->SetColorAttachmentSrcLayout(0, Texture::Layout::Undefined, std::nullopt);
      builder->SetColorAttachmentDstLayout(0, Texture::Layout::PresentSrc, std::nullopt);

      builder->SetDepthAttachmentFormat(Texture::Format::DEPTH_U16);
      builder->SetDepthAttachmentSrcLayout(Texture::Layout::Undefined, std::nullopt);
      builder->SetDepthAttachmentDstLayout(Texture::Layout::DepthStencilAttachment, std::nullopt);

      render_pass = builder->Build();

      fence = render_device->CreateFence();
    }

    std::shared_ptr<RenderDevice> render_device;
    std::shared_ptr<CommandPool> command_pool;
    std::unique_ptr<CommandBuffer> command_buffer;
    std::shared_ptr<RenderPass> render_pass;
    std::unique_ptr<Fence> fence;
};

int main() {
  zephyr::get_logger().InstallSink(std::make_shared<LoggerConsoleSink>());

  auto window = std::make_unique<MainWindow>();
  window->Run();

  return 0;
}
