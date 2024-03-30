
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <span>

namespace zephyr {

  struct RenderObject {
    Matrix4 local_to_world;
  };

  class RenderBackend {
    public:
      virtual ~RenderBackend() = default;

      /// Just a quick thing for testing the rendering.
      virtual void Render(const Matrix4& projection, std::span<const RenderObject> render_objects) = 0;
  };

} // namespace zephyr
