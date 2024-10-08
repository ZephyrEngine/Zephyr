
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <zephyr/math/rotation.hpp>
#include <zephyr/math/vector.hpp>

namespace zephyr {

class SceneNode;

class Transform3D {
  public:
    explicit Transform3D(SceneNode* node) : m_node{node} {
      UpdateLocal();
      UpdateWorld();
    }

    [[nodiscard]] const Vector3& GetPosition() const {
      return m_position;
    }

    void SetPosition(const Vector3& position) {
      m_position = position;
      SignalNodeTransformChanged();
    }

    [[nodiscard]] const Vector3& GetScale() const {
      return m_scale;
    }

    void SetScale(const Vector3& scale) {
      m_scale = scale;
      SignalNodeTransformChanged();
    }

    [[nodiscard]] const Quaternion& GetRotation() const {
      return m_rotation;
    }

    void SetRotation(const Quaternion& rotation) {
      m_rotation = rotation;
      SignalNodeTransformChanged();
    }

    [[nodiscard]] const Matrix4& GetLocal() const {
      return m_local_matrix;
    }

    [[nodiscard]] const Matrix4& GetWorld() const {
      return m_world_matrix;
    }

    void UpdateLocal();
    void UpdateWorld();

  private:
    void SignalNodeTransformChanged();

    SceneNode* m_node;
    Vector3 m_position{};
    Vector3 m_scale{1.0f, 1.0f, 1.0f};
    Quaternion m_rotation{};
    Matrix4 m_local_matrix{};
    Matrix4 m_world_matrix{};
};

} // namespace zephyr
