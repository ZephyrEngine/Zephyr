
#pragma once

#include <zephyr/math/frustum.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/scene/component.hpp>
#include <zephyr/float.hpp>

namespace zephyr {

class PerspectiveCameraComponent : public Component {
  public:
    PerspectiveCameraComponent(f32 field_of_view, f32 aspect_ratio, f32 near, f32 far) {
      Setup(field_of_view, aspect_ratio, near, far);
    }

    void Setup(f32 field_of_view, f32 aspect_ratio, f32 near, f32 far) {
      m_field_of_view = field_of_view;
      m_aspect_ratio = aspect_ratio;
      m_near = near;
      m_far = far;
      m_needs_update = true;
    }

    [[nodiscard]] const Matrix4& GetProjectionMatrix() const {
      if(m_needs_update) {
        UpdateProjectionMatrixAndFrustum();
      }
      return m_projection_matrix;
    }

    [[nodiscard]] const Frustum& GetFrustum() const {
      if(m_needs_update) {
        UpdateProjectionMatrixAndFrustum();
      }
      return m_frustum;
    }

    [[nodiscard]] f32 GetFieldOfView() const {
      return m_field_of_view;
    }

    void SetFieldOfView(f32 field_of_view) {
      m_field_of_view = field_of_view;
      m_needs_update = true;
    }

    [[nodiscard]] f32 GetAspectRatio() const {
      return m_aspect_ratio;
    }

    void SetAspectRatio(f32 aspect_ratio) {
      m_aspect_ratio = aspect_ratio;
      m_needs_update = true;
    }

    [[nodiscard]] f32 GetNear() const {
      return m_near;
    }

    void SetNear(f32 near) {
      m_near = near;
      m_needs_update = true;
    }

    [[nodiscard]] f32 GetFar() const {
      return m_far;
    }

    void SetFar(f32 far) {
      m_far = far;
      m_needs_update = true;
    }

  private:
    void UpdateProjectionMatrixAndFrustum() const {
      m_projection_matrix = Matrix4::PerspectiveVK(m_field_of_view, m_aspect_ratio, m_near, m_far);

      const f32 x = 1.f / m_projection_matrix.X().X();
      const f32 y = 1.f / m_projection_matrix.Y().Y();

      m_frustum.SetPlane(Frustum::Side::NZ, Plane{Vector3{ 0,  0, -1}, -m_near});
      m_frustum.SetPlane(Frustum::Side::PZ, Plane{Vector3{ 0,  0,  1}, -m_far });
      m_frustum.SetPlane(Frustum::Side::NX, Plane{Vector3{ 1 , 0, -x}.Normalize(), 0});
      m_frustum.SetPlane(Frustum::Side::PX, Plane{Vector3{-1 , 0, -x}.Normalize(), 0});
      m_frustum.SetPlane(Frustum::Side::NY, Plane{Vector3{ 0,  1, -y}.Normalize(), 0});
      m_frustum.SetPlane(Frustum::Side::PY, Plane{Vector3{ 0, -1, -y}.Normalize(), 0});

      m_needs_update = false;
    }

    f32 m_field_of_view{};
    f32 m_aspect_ratio{};
    f32 m_near{};
    f32 m_far{};

    mutable bool m_needs_update{};
    mutable Matrix4 m_projection_matrix{};
    mutable Frustum m_frustum{};
};

} // namespace zephyr
