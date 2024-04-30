
#pragma once

#include <zephyr/renderer/resource/geometry.hpp>
#include <zephyr/scene/component.hpp>
#include <memory>

namespace zephyr {

  struct MeshComponent : Component {
    MeshComponent() = default;
    MeshComponent(std::shared_ptr<Geometry> geometry) : geometry{std::move(geometry)} {}

    std::shared_ptr<Geometry> geometry;
  };

} // namespace zephyr
