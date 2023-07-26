
#include <zephyr/scene/component.hpp>
#include <zephyr/renderer/mesh.hpp>
#include <memory>

namespace zephyr {

  struct MeshComponent : Component {
    std::shared_ptr<Mesh3D> mesh;

    explicit MeshComponent(std::shared_ptr<Mesh3D> mesh) : mesh{std::move(mesh)} {}
  };


} // namespace zephyr
