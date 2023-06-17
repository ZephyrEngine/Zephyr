
#pragma once

#include <functional>
#include <string_view>
#include <zephyr/window/window.hpp>

namespace zephyr {

  class WindowImpl {
    public:
      virtual ~WindowImpl() = default;

      virtual void SetWindowTitle(std::string_view window_title) = 0;
      virtual auto GetSize() -> Size = 0;
      virtual void SetSize(int width, int height) = 0;
      virtual bool PollEvents() = 0;
      virtual std::shared_ptr<RenderDevice>& GetRenderDevice() = 0;
      virtual std::shared_ptr<SwapChain>& GetSwapChain() = 0;

      /**
       * @todo: find and implement a better solution for passing event data
       * from the window implementation to the window.
       */
      virtual void SetOnResizeCallback(std::function<void(int, int)> callback) = 0;
  };

} // namespace zephyr