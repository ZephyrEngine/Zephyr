
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/window/swap_chain.hpp>
#include <memory>
#include <string_view>

namespace zephyr {

  struct Size {
    int width;
    int height;
  };

  class Window {
    public:
      Window();
      Window(std::string_view window_title, int width, int height);
     ~Window();

      void SetWindowTitle(std::string_view window_title);
      auto GetWindowSize() -> Size;
      void SetWindowSize(int width, int height);
      std::shared_ptr<RenderDevice>& GetRenderDevice();
      std::shared_ptr<SwapChain>& GetSwapChain();
      void Run();

    protected:
      virtual void OnFrame() {}
      virtual void OnResize(int width, int height) {}

    private:
      void CreateWindowImplementation();

      class WindowImpl* impl = nullptr;
  };

} // namespace zephyr