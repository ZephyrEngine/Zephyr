
#include <zephyr/window/window.hpp>

#include "vulkan/vulkan_window.hpp"

namespace zephyr {

  Window::Window() {
    CreateWindowImplementation();
    SetWindowTitle("Zephyr Window");
    SetWindowSize(1600, 900);
  }

  Window::Window(std::string_view window_title, int width, int height) {
    CreateWindowImplementation();
    SetWindowTitle(window_title);
    SetWindowSize(width, height);
  }

  Window::~Window() {
    delete impl;
  }

  void Window::SetWindowTitle(std::string_view window_title) {
    impl->SetWindowTitle(window_title);
  }

  auto Window::GetWindowSize() -> Size {
    return impl->GetSize();
  }

  void Window::SetWindowSize(int width, int height) {
    impl->SetSize(width, height);
  }

  std::shared_ptr<RenderDevice>& Window::GetRenderDevice() {
    return impl->GetRenderDevice();
  }

  std::shared_ptr<SwapChain>& Window::GetSwapChain() {
    return impl->GetSwapChain();
  }

  void Window::Run() {
    while (true) {
      if (!impl->PollEvents()) {
        return;
      }

      OnFrame();
    }
  }

  void Window::CreateWindowImplementation() {
    impl = new VulkanWindow{};

    impl->SetOnResizeCallback([this](int width, int height) { OnResize(width, height); });
  }

} // namespace zephyr