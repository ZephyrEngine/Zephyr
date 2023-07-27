
#include <zephyr/scene/node.hpp>
#include <zephyr/scene/transform.hpp>

namespace zephyr {

  void Transform3D::UpdateLocal() {
    m_local_matrix = m_rotation.GetAsMatrix4();
    m_local_matrix.X() *= m_scale.X();
    m_local_matrix.Y() *= m_scale.Y();
    m_local_matrix.Z() *= m_scale.Z();
    m_local_matrix.W() = Vector4{m_position, 1.0f};
  }

  void Transform3D::UpdateWorld() {
    SceneNode* parent = m_node->GetParent();

    if(parent) {
      m_world_matrix = parent->GetTransform().GetWorld() * m_local_matrix;
    } else {
      m_world_matrix = m_local_matrix;
    }
  }

} // namespace zephyr
