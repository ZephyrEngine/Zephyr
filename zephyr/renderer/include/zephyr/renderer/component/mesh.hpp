
#pragma once

#include <zephyr/renderer/resource/geometry.hpp>
#include <zephyr/renderer/resource/material.hpp>
#include <zephyr/scene/component.hpp>
#include <memory>

namespace zephyr {

  struct MeshComponent : Component {
    MeshComponent() = default;
    MeshComponent(std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material) : geometry{std::move(geometry)}, material{std::move(material)} {}

    std::shared_ptr<Geometry> geometry;
    std::shared_ptr<Material> material;
  };

} // namespace zephyr
