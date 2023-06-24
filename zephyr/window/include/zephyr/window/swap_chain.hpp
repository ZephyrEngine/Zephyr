
#pragma once

namespace zephyr {

  class SwapChain {
    public:
      virtual ~SwapChain() = default;

      virtual std::shared_ptr<RenderTarget>& AcquireNextRenderTarget() = 0;
      [[nodiscard]] virtual size_t GetNumberOfSwapChainImages() const = 0;
      virtual void Present() = 0;
  };

} // namespace zephyr