
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <zephyr/math/quaternion.hpp>
#include <zephyr/math/vector.hpp>
#include <zephyr/event.hpp>
#include <zephyr/float.hpp>

namespace zephyr {

  /**
   * Represents a rotation around an arbitrary axis in 3D space.
   * Underlying to the rotation is a quaternion encoding the current axis and angle.
   */
  class Rotation {
    public:
      /**
       * Default constructor. By default the Rotation is initialized to not rotate.
       */
      Rotation() {
        SetFromQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
      }

      /**
       * Construct a Rotation from extrinsic x-y-z (intrinsic z-y'-x'') Tait-Bryan angles.
       * @param x X angle
       * @param y Y angle
       * @param z Z angle
       */
      Rotation(f32 x, f32 y, f32 z) {
        SetFromEuler(x, y, z);
      }

      /**
       * Construct a Rotation from a quaternion.
       * @param quaternion the Quaternion
       */
      explicit Rotation(const Quaternion& quaternion) {
        SetFromQuaternion(quaternion);
      }

      /// @returns a reference to an event that is emitted whenever the rotation changes.
      [[nodiscard]] VoidEvent& OnChange() const {
        return m_event_on_change;
      }

      /// @returns the rotation in quaternion form
      [[nodiscard]] const Quaternion& GetAsQuaternion() const {
        return m_quaternion;
      }

      /**
       * Set the rotation from a quaternion.
       * @param quaternion the Quaternion
       */
      void SetFromQuaternion(const Quaternion& quaternion) {
        m_quaternion = quaternion;
        MarkQuaternionAsChanged();
      }

      /**
       * Set the rotation from a quaternion.
       * @param w the w-component of the quaternion
       * @param x the x-component of the quaternion
       * @param y the y-component of the quaternion
       * @param z the z-component of the quaternion
       */
      void SetFromQuaternion(f32 w, f32 x, f32 y, f32 z) {
        SetFromQuaternion({w, x, y, z});
      }

      /// @returns the rotation in 4x4 matrix form
      [[nodiscard]] const Matrix4& GetAsMatrix4() const {
        UpdateMatrixFromQuaternion();
        return m_matrix;
      }

      /**
       * Set the rotation from a 4x4 matrix
       * @param matrix the 4x4 matrix
       */
      void SetFromMatrix4(const Matrix4& matrix) {
        m_quaternion = Quaternion::FromRotationMatrix(matrix);
        // The 4x4 matrix is likely to be read and copying it now is faster than reconstructing it later.
        m_matrix = matrix;
        m_needs_euler_refresh = true;
        m_event_on_change.Emit();
      }

      /**
       * Returns the rotation in extrinsic x-y-z (intrinsic z-y'-x'') Tait-Bryan angles form.
       * This is an expensive operation because the angles may need to be reconstructed.
       * Due to the reconstruction, there also is no guarantee that calling
       * {@link #GetAsEuler} after {@link #SetFromEuler} will return the original angles.
       * @returns the Tait-Bryan angles
       */
      const Vector3& GetAsEuler() {
        UpdateEulerFromQuaternion();
        return m_euler;
      }

      /**
       * Set the rotation from extrinsic x-y-z (intrinsic z-y'-x'') Tait-Bryan angles.
       * @param x X angle
       * @param y Y angle
       * @param z Z angle
       */
      void SetFromEuler(f32 x, f32 y, f32 z) {
        const f32 half_x = x * 0.5f;
        const f32 cos_x = std::cos(half_x);
        const f32 sin_x = std::sin(half_x);

        const f32 half_y = y * 0.5f;
        const f32 cos_y = std::cos(half_y);
        const f32 sin_y = std::sin(half_y);

        const f32 half_z = z * 0.5f;
        const f32 cos_z = std::cos(half_z);
        const f32 sin_z = std::sin(half_z);

        const f32 cos_z_cos_y = cos_z * cos_y;
        const f32 sin_z_cos_y = sin_z * cos_y;
        const f32 sin_z_sin_y = sin_z * sin_y;
        const f32 cos_z_sin_y = cos_z * sin_y;

        SetFromQuaternion(
          cos_z_cos_y * cos_x + sin_z_sin_y * sin_x,
          cos_z_cos_y * sin_x - sin_z_sin_y * cos_x,
          sin_z_cos_y * sin_x + cos_z_sin_y * cos_x,
          sin_z_cos_y * cos_x - cos_z_sin_y * sin_x
        );
      }

      /**
       * Set the rotation from extrinsic x-y-z (intrinsic z-y'-x'') Tait-Bryan angles.
       * @param euler the Tait-Bryan euler angles
       */
      void SetFromEuler(Vector3 euler) {
        SetFromEuler(euler.X(), euler.Y(), euler.Z());
      }

    private:
      /// Signal internally that the quaternion has changed
      void MarkQuaternionAsChanged() {
        m_needs_matrix_refresh = true;
        m_needs_euler_refresh = true;
        m_event_on_change.Emit();
      }

      /// Update the 4x4 matrix from the quaternion.
      void UpdateMatrixFromQuaternion() const {
        if(m_needs_matrix_refresh) {
          m_matrix = m_quaternion.ToRotationMatrix();
          m_needs_matrix_refresh = false;
        }
      }

      /// Update the euler angles from the quaternion.
      void UpdateEulerFromQuaternion() const {
        if(!m_needs_euler_refresh) {
          return;
        }

        // @todo: reconstruct the angles from the quaternion instead of the 4x4 matrix (which possibly needs to be updated first).
        static constexpr f32 k_cos0_threshold = 1.0f - 1e-6f;

        UpdateMatrixFromQuaternion();

        const f32 sin_y = -m_matrix[0][2];

        m_euler.Y() = std::asin(std::clamp(sin_y, -1.0f, +1.0f));

        // Guard against gimbal lock when Y=-90°/+90° (X and Z rotate around the same axis).
        if(std::abs(sin_y) <= k_cos0_threshold) {
          m_euler.X() = std::atan2(m_matrix[1][2], m_matrix[2][2]);
          m_euler.Z() = std::atan2(m_matrix[0][1], m_matrix[0][0]);
        } else {
          m_euler.X() = std::atan2(m_matrix[1][0], m_matrix[2][0]);
          m_euler.Z() = 0.0f;
        }

        m_needs_euler_refresh = false;
      }

      Quaternion m_quaternion{}; ///< the underlying rotation encoding the current axis and angle

      mutable Matrix4 m_matrix{}; ///< a 4x4 rotation matrix which is updated from the quaternion on demand.
      mutable Vector3 m_euler{}; ///< a vector of euler angles which is updated from the quaternion on demand.
      mutable bool m_needs_matrix_refresh{true}; ///< true when the 4x4 matrix (#{@link m_matrix}) is outdated and false otherwise.
      mutable bool m_needs_euler_refresh{true}; ///< true whe euler angles (#{@link m_euler}) are outdated and false otherwise.
      mutable VoidEvent m_event_on_change{}; ///< An event that is emitted when the rotation has changed.
  };

} // namespace zephyr
