
#include <zephyr/scene/component.hpp>
#include <zephyr/renderer/material/material.hpp>
#include <zephyr/renderer/mesh.hpp>
#include <memory>

#pragma once

namespace zephyr {

  struct MeshComponent : Component {
    std::shared_ptr<Mesh3D> mesh;
    std::shared_ptr<Material> material;

    explicit MeshComponent(std::shared_ptr<Mesh3D> mesh, std::shared_ptr<Material> material)
        : mesh{std::move(mesh)}, material{std::move(material)} {}
  };


} // namespace zephyr
